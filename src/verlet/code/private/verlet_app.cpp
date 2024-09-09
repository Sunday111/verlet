#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "coloring/tick_color/tick_color_strategy.hpp"
#include "gui/app_gui.hpp"
#include "klgl/events/event_listener_method.hpp"
#include "klgl/events/event_manager.hpp"
#include "klgl/events/mouse_events.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/reflection/matrix_reflect.hpp"  // IWYU pragma: keep
#include "klgl/shader/shader.hpp"
#include "klgl/texture/procedural_texture_generator.hpp"
#include "klgl/texture/texture.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"

namespace verlet
{

edt::FloatRange2Df Camera::ComputeRange(const edt::FloatRange2Df& world_range) const
{
    const auto half_world_extent = world_range.Extent() / 2;
    const auto half_camera_extent = half_world_extent / GetZoom();
    return edt::FloatRange2Df::FromMinMax(GetEye() - half_camera_extent, GetEye() + half_camera_extent);
}

void Camera::Update(const edt::FloatRange2Df& world_range)
{
    range_ = ComputeRange(world_range);
    if (zoom_animation_ && zoom_animation_->Update(zoom_)) zoom_animation_ = std::nullopt;
    if (eye_animation_ && eye_animation_->Update(eye_)) eye_animation_ = std::nullopt;
}

void Camera::Zoom(const float delta)
{
    if (animate)
    {
        float final_value = zoom_ + delta;
        if (zoom_animation_)
        {
            final_value = zoom_animation_->final_value + delta;
        }

        zoom_animation_ = ValueAnimation{
            .start_value = zoom_,
            .final_value = final_value,
            .duration_seconds = zoom_animation_diration_seconds,
        };
    }
    else
    {
        zoom_ += delta;
    }
}

void Camera::Pan(const Vec2f& delta)
{
    if (animate)
    {
        edt::Vec2f final_value = eye_ + delta;
        if (eye_animation_)
        {
            final_value = eye_animation_->final_value + delta;
        }

        eye_animation_ = ValueAnimation{
            .start_value = eye_,
            .final_value = final_value,
            .duration_seconds = zoom_animation_diration_seconds,
        };
    }
    else
    {
        eye_ += delta;
    }
}

VerletApp::VerletApp()
{
    event_listener_ = klgl::events::EventListenerMethodCallbacks<&VerletApp::OnMouseScroll>::CreatePtr(this);
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

    klgl::OpenGl::SetClearColor({});
    GetWindow().SetSize(2000, 1000);
    GetWindow().SetTitle("Verlet");

    shader_ = std::make_unique<klgl::Shader>("verlet.shader.json");
    shader_->Use();

    {
        // Generate circle mask texture
        constexpr auto size = Vec2<size_t>{} + 128;
        texture_ = klgl::Texture::CreateEmpty(size, klgl::GlTextureInternalFormat::R8);
        const auto pixels = klgl::ProceduralTextureGenerator::CircleMask(size, 2);
        texture_->SetPixels<klgl::GlPixelBufferLayout::R>(std::span{pixels});
        klgl::OpenGl::SetTextureMinFilter(klgl::GlTargetTextureType::Texture2d, klgl::GlTextureFilter::Nearest);
        klgl::OpenGl::SetTextureMagFilter(klgl::GlTargetTextureType::Texture2d, klgl::GlTextureFilter::Linear);
    }

    instance_painter_.Initialize();
}

void VerletApp::UpdateWorldRange()
{
    // Handle aspect ratio here: if some side of viewport becomes bigger than other -
    // we react by increasing the world coordinate range on that axis
    const auto [smaller, bigger, ratio] = [&]()
    {
        if (auto [width, height] = GetWindow().GetSize2f().Tuple(); width > height)
        {
            return std::tuple{&world_range_.y, &world_range_.x, width / height};
        }
        else
        {
            return std::tuple{&world_range_.x, &world_range_.y, height / width};
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
    adjust_range(world_range_.x, sim_area.x);
    adjust_range(world_range_.y, sim_area.y);
    solver.SetSimArea(sim_area);
}

void VerletApp::UpdateCamera()
{
    camera_.Update(world_range_);

    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        Vec2f offset{};
        if (ImGui::IsKeyDown(ImGuiKey_W)) offset.y() += 1.f;
        if (ImGui::IsKeyDown(ImGuiKey_S)) offset.y() -= 1.f;
        if (ImGui::IsKeyDown(ImGuiKey_D)) offset.x() += 1.f;
        if (ImGui::IsKeyDown(ImGuiKey_A)) offset.x() -= 1.f;

        camera_.Pan((GetLastFrameDurationSeconds() * camera_.GetRange().Extent() * offset) * camera_.pan_speed);
    }
}

void VerletApp::UpdateSimulation()
{
    if (tool_)
    {
        tool_->Tick();
    }

    if (emitter_.enabled && solver.objects.ObjectsCount() <= emitter_.max_objects_count)
    {
        auto color_fn = spawn_color_strategy_->GetColorFunction();
        for (uint32_t i{40}; i--;)
        {
            const float y = 50.f + 1.02f * static_cast<float>(i);

            {
                auto [id, object] = solver.objects.Alloc();
                object.position = {0.6f, y};
                object.old_position = {0.4f, y};
                object.movable = true;
                object.color = color_fn(object);
            }

            {
                auto [id, object] = solver.objects.Alloc();
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
    UpdateRenderTransforms();
    RenderWorld();
    AppGUI{*this}.Render();
}

void VerletApp::UpdateRenderTransforms()
{
    const auto screen_range = edt::FloatRange2Df::FromMinMax({}, GetWindow().GetSize2f());
    const auto view_range = edt::FloatRange2Df::FromMinMax(Vec2f{} - 1, Vec2f{} + 1);
    const auto camera_to_world_vector = world_range_.Uniform(.5f) - camera_.GetEye();
    const auto camera_extent = camera_.GetRange().Extent();

    world_to_camera_ = edt::Math::TranslationMatrix(camera_to_world_vector);
    auto camera_to_view_ = edt::Math::ScaleMatrix(view_range.Extent() / camera_extent);
    world_to_view_ = camera_to_view_.MatMul(world_to_camera_);

    const auto screen_to_view =
        edt::Math::TranslationMatrix(Vec2f{} - 1).MatMul(edt::Math::ScaleMatrix(2 / screen_range.Extent()));
    const auto view_to_camera = edt::Math::ScaleMatrix(camera_extent / view_range.Extent());
    const auto camera_to_world = edt::Math::TranslationMatrix(-camera_to_world_vector);
    screen_to_world_ = camera_to_world.MatMul(view_to_camera).MatMul(screen_to_view);
}

void VerletApp::RenderWorld()
{
    ObjectColorFunction color_function = [](const VerletObject& object)
    {
        return object.color;
    };
    if (tick_color_strategy_) color_function = tick_color_strategy_->GetColorFunction();

    const klgl::ScopeAnnotation annotation("Render World");

    instance_painter_.num_objects_ = 0;

    auto paint_instanced_object = [&](const VerletObject& object) mutable
    {
        const auto& color = color_function(object);
        // const auto screen_pos = edt::Math::TransformPos(world_to_view_, object.position);
        // const auto screen_size = edt::Math::TransformVector(world_to_view_, object.GetRadius() + Vec2f{});
        // instance_painter_.DrawObject(screen_pos, color, screen_size);
        instance_painter_.DrawObject(object.position, color, object.GetRadius() + Vec2f{});
    };

    perf_stats_.render.total = edt::MeasureTime(
        [&]
        {
            klgl::OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            klgl::OpenGl::EnableBlending();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            perf_stats_.render.set_circle_loop = edt::MeasureTime(
                [&]
                {
                    for (const VerletObject& object : solver.objects.Objects())
                    {
                        paint_instanced_object(object);
                    }
                });

            if (tool_)
            {
                tool_->DrawInWorld();
            }

            shader_->Use();
            shader_->SetUniform(u_world_to_view_, world_to_view_.Transposed());
            shader_->SendUniforms();

            texture_->Bind();
            instance_painter_.Render();
        });
}

void VerletApp::OnMouseScroll(const klgl::events::OnMouseScroll& event)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        camera_.Zoom(event.value.y() * camera_.zoom_speed);
    }
}

void VerletApp::SetBackgroundColor(const Vec3f& background_color)
{
    if (background_color_ == background_color) return;
    background_color_ = background_color;
    const auto& v = background_color_;
    klgl::OpenGl::SetClearColor(Vec4f{v.x(), v.y(), v.z(), 1.f});
}

Vec2f VerletApp::GetMousePositionInWorldCoordinates() const
{
    const auto screen_range = edt::FloatRange2Df::FromMinMax({}, GetWindow().GetSize2f());  // 0 -> resolution
    auto [x, y] = ImGui::GetMousePos();
    y = screen_range.y.Extent() - y;
    return edt::Math::TransformPos(screen_to_world_, Vec2f{x, y});
}
}  // namespace verlet
