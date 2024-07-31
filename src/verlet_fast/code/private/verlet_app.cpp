#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "coloring/tick_color/tick_color_strategy.hpp"
#include "gui/app_gui.hpp"
#include "klgl/events/event_listener_method.hpp"
#include "klgl/events/event_manager.hpp"
#include "klgl/events/window_events.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"

namespace verlet
{

VerletApp::VerletApp()
{
    event_listener_ = klgl::events::EventListenerMethodCallbacks<&VerletApp::OnWindowResize>::CreatePtr(this);
    GetEventManager().AddEventListener(*event_listener_);
}

VerletApp::~VerletApp()
{
    GetEventManager().RemoveListener(event_listener_.get());
}

void VerletApp::Initialize()
{
    Super::Initialize();
    spawn_color_strategy_ = std::make_unique<SpawnColorStrategyRainbow>(*this);
    InitializeRendering();
}

void VerletApp::Tick()
{
    Super::Tick();
    UpdateWorldRange();
    UpdateCamera();
    UpdateSimulation();
    Render();
}

void VerletApp::InitializeRendering()
{
    SetTargetFramerate({60.f});

    // klgl::OpenGl::SetClearColor(Vec4f{255, 245, 153, 255} / 255.f);
    klgl::OpenGl::SetClearColor({});
    GetWindow().SetSize(1000, 1000);
    GetWindow().SetTitle("Verlet");

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    klgl::Shader::shaders_dir_ = shaders_dir;

    shader_ = std::make_unique<klgl::Shader>("just_color.shader.json");
    shader_->Use();

    circle_painter_.Initialize();
}

void VerletApp::UpdateWorldRange()
{
    // Handle aspect ratio here: if some side of viewport becomes bigger than other -
    // we react by increasing the world coordinate range on that axis
    const auto [smaller, bigger, ratio] = [&]()
    {
        if (auto [width, height] = GetWindow().GetSize2f().Tuple(); width > height)
        {
            return std::tuple{&world_range.y, &world_range.x, width / height};
        }
        else
        {
            return std::tuple{&world_range.x, &world_range.y, height / width};
        }
    }();

    *smaller = kMinSideRange;
    *bigger = smaller->Enlarged(smaller->Extent() * (ratio - 1.f) * 0.5f);

    static constexpr float kMaxExtentChangePerTick = 0.5f;
    auto adjust_range = [](const edt::FloatRange<float>& world, edt::FloatRange<float>& sim)
    {
        if (world.begin < sim.begin)
        {
            sim.begin = world.begin;
        }
        else
        {
            sim.begin += std::min(kMaxExtentChangePerTick, world.begin - sim.begin);
        }

        if (world.end > sim.end)
        {
            sim.end = world.end;
        }
        else
        {
            sim.end -= std::min(kMaxExtentChangePerTick, sim.end - world.end);
        }
    };

    auto sim_area = solver.GetSimArea();
    adjust_range(world_range.x, sim_area.x);
    adjust_range(world_range.y, sim_area.y);
    solver.SetSimArea(sim_area);
}

void VerletApp::UpdateCamera() {}

void VerletApp::UpdateSimulation()
{
    if (tool_)
    {
        tool_->Tick();
    }

    // Emitter
    if (enable_emitter_ && solver.objects.ObjectsCount() <= emitter_max_objects_count_)
    {
        auto color_fn = spawn_color_strategy_->GetColorFunction();
        for (uint32_t i{40}; i--;)
        {
            const float y = 50.f + 1.02f * static_cast<float>(i);

            {
                auto [aid, object] = solver.objects.Alloc();
                object.position = {0.6f, y};
                object.old_position = {0.4f, y};
                object.movable = true;
                object.color = color_fn(object);
            }

            {
                auto [bid, object] = solver.objects.Alloc();
                object.position = {-0.6f, y};
                object.old_position = {-0.4f, y};
                object.movable = true;
                object.color = color_fn(object);
            }
        }
    }

    perf_stats_.sim_update = solver.Update();
}

void VerletApp::Render()
{
    RenderWorld();
    AppGUI{*this}.Render();
}

static constexpr edt::Mat3f TranslationMatrix(const Vec2f translation)
{
    auto m = edt::Mat3f::Identity();
    m(0, 2) = translation.x();
    m(1, 2) = translation.y();
    return m;
}

static constexpr edt::Mat3f ScaleMatrix(const Vec2f scale)
{
    auto m = edt::Mat3f::Identity();
    m(0, 0) = scale.x();
    m(1, 1) = scale.y();
    return m;
}

constexpr edt::Mat3f WorldToCameraTransform(Vec2f camera_pos, Vec2f camera_scale)
{
    auto t = TranslationMatrix(0.f - camera_pos);
    auto s = ScaleMatrix(camera_scale);
    return s.MatMul(t);
}

constexpr edt::Mat3f AffineTransform(const edt::FloatRange2Df& from, const edt::FloatRange2Df& to)
{
    auto t = TranslationMatrix(from.Uniform(0.5f) - to.Uniform(0.5f));
    auto s = ScaleMatrix(from.Extent() / to.Extent());
    return s.MatMul(t);
}

void VerletApp::RenderWorld()
{
    ObjectColorFunction color_function = [](const VerletObject& object)
    {
        return object.color;
    };
    if (tick_color_strategy_) color_function = tick_color_strategy_->GetColorFunction();

    const klgl::ScopeAnnotation annotation("Render World");

    const auto world_space = world_range;
    const auto camera_space =
        edt::FloatRange2Df::FromMinMax(world_space.Min() / camera_zoom_, world_space.Max() / camera_zoom_)
            .Shifted(camera_eye_);
    const auto screen_space = edt::FloatRange2Df::FromMinMax(Vec2f{} - 1, Vec2f{} + 1);
    auto world_to_camera = AffineTransform(world_space, camera_space);
    auto camera_to_view = edt::Math::MakeTransform(camera_space, screen_space);
    auto world_to_view = camera_to_view.MatMul(world_to_camera);

    circle_painter_.num_circles_ = 0;

    auto paint_instanced_object = [&, next_instance_index = 0uz](const VerletObject& object) mutable
    {
        const auto screen_pos = TransformPos(world_to_view, object.position);
        const auto screen_size = TransformVector(world_to_view, object.GetRadius() + Vec2f{});
        const auto& color = color_function(object);
        circle_painter_.SetCircle(next_instance_index++, screen_pos, color, screen_size);
    };

    perf_stats_.render.total = edt::MeasureTime(
        [&]
        {
            klgl::OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            perf_stats_.render.set_circle_loop = edt::MeasureTime(
                [&]
                {
                    for (const VerletObject& object : solver.objects.Objects())
                    {
                        paint_instanced_object(object);
                    }
                });

            shader_->Use();
            circle_painter_.Render();
        });
}

void VerletApp::OnWindowResize(const klgl::events::OnWindowResize&) {}
}  // namespace verlet
