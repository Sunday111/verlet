#include "verlet_app.hpp"

#include <nlohmann/json.hpp>

#include "EverydayTools/Time/MeasureTime.hpp"
#include "coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "coloring/tick_color/tick_color_strategy.hpp"
#include "gui/app_gui.hpp"
#include "klgl/error_handling.hpp"
#include "klgl/events/event_listener_method.hpp"
#include "klgl/events/event_manager.hpp"
#include "klgl/events/mouse_events.hpp"
#include "klgl/filesystem/filesystem.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/reflection/matrix_reflect.hpp"  // IWYU pragma: keep
#include "klgl/shader/shader.hpp"
#include "klgl/texture/procedural_texture_generator.hpp"
#include "klgl/texture/texture.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"
#include "verlet/json/json_helpers.hpp"
#include "verlet/json/json_keys.hpp"

namespace verlet
{

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
    GetWindow().SetSize(1920, 1080);
    UpdateWorldRange(std::numeric_limits<float>::max());
    GetWindow().SetTitle("Verlet");

    shader_ = std::make_unique<klgl::Shader>("verlet");
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

void VerletApp::UpdateWorldRange(float max_extent_change)
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

    auto adjust_range = [&max_extent_change](const edt::FloatRange<float>& world, edt::FloatRange<float>& sim)
    {
        if (world.begin < sim.begin)
        {
            sim.begin = world.begin;
        }
        else
        {
            sim.begin += std::min(max_extent_change, world.begin - sim.begin);
        }

        if (world.end > sim.end)
        {
            sim.end = world.end;
        }
        else
        {
            sim.end -= std::min(max_extent_change, sim.end - world.end);
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

    // Update emitters
    {
        // Delete pending kill emitters
        {
            auto r = std::ranges::remove(emitters_, true, &Emitter::pending_kill);
            emitters_.erase(r.begin(), r.end());
        }

        // Iterate only through emitters that existed before
        for (const size_t emitter_index : std::views::iota(size_t{0}, emitters_.size()))
        {
            auto& emitter = *emitters_[emitter_index];
            emitter.Tick(*this);

            if (emitter.clone_requested)
            {
                emitter.clone_requested = false;
                auto cloned = emitter.Clone();
                cloned->ResetRuntimeState();
                emitters_.push_back(std::move(cloned));
            }
        }
    }

    perf_stats_.sim_update = solver.Update();
    time_steps_++;
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

void VerletApp::SaveAppState(const std::filesystem::path& path) const
{
    klgl::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            static constexpr int indent_size = 4;
            static constexpr char indent_char = ' ';
            nlohmann::json json = JSONHelpers::AppStateToJSON(*this);
            klgl::Filesystem::WriteFile(path, json.dump(indent_size, indent_char));
        });
}

void VerletApp::LoadAppState(const std::filesystem::path& path)
{
    klgl::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            std::string content;
            klgl::Filesystem::ReadFile(path, content);

            const auto json = nlohmann::json::parse(content);

            auto window_size = JSONHelpers::Vec2iFromJSON(json[JSONKeys::kWindowSize]).Cast<uint32_t>();
            max_objects_count_ = json[JSONKeys::kMaxObjectsCount];
            GetWindow().SetSize(window_size.x(), window_size.y());

            DeleteAllEmitters();

            for (const auto& emitter_json : json[JSONKeys::kEmitters])
            {
                AddEmitter(JSONHelpers::EmitterFromJSON(emitter_json));
            }
        });
}

void VerletApp::SavePositions(const std::filesystem::path& path) const
{
    klgl::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            std::string buffer;
            auto inserter = std::back_inserter(buffer);

            fmt::format_to(inserter, "{}\n", solver.objects.ObjectsCount());
            for (const auto& object : solver.objects.Objects())
            {
                fmt::format_to(inserter, "{} {}\n", object.position.x(), object.position.y());
            }

            klgl::Filesystem::WriteFile(path, buffer);
        });
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

void VerletApp::AddEmitter(std::unique_ptr<Emitter> emitter)
{
    emitters_.push_back(std::move(emitter));
}

Vec2f VerletApp::GetMousePositionInWorldCoordinates() const
{
    const auto screen_range = edt::FloatRange2Df::FromMinMax({}, GetWindow().GetSize2f());  // 0 -> resolution
    auto [x, y] = ImGui::GetMousePos();
    y = screen_range.y.Extent() - y;
    return edt::Math::TransformPos(screen_to_world_, Vec2f{x, y});
}

void VerletApp::DeleteAllEmitters()
{
    std::ranges::fill(GetEmitters() | std::views::transform(&Emitter::pending_kill), true);
}

void VerletApp::EnableAllEmitters()
{
    std::ranges::fill(GetEmitters() | std::views::transform(&Emitter::enabled), true);
}

void VerletApp::DisableAllEmitters()
{
    std::ranges::fill(GetEmitters() | std::views::transform(&Emitter::enabled), false);
}
}  // namespace verlet
