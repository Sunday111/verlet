#include "verlet_app.hpp"

#include <nlohmann/json.hpp>
#include <numbers>

#include "coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "coloring/tick_color/tick_color_strategy.hpp"
#include "edt/time/measure_time.hpp"
#include "gui/app_gui.hpp"
#include "klvk/error_handling.hpp"
#include "klvk/events/event_listener_method.hpp"
#include "klvk/events/event_manager.hpp"
#include "klvk/events/mouse_events.hpp"
#include "klvk/filesystem/filesystem.hpp"
#include "klvk/reflection/matrix_reflect.hpp"  // IWYU pragma: keep
#include "klvk/texture/procedural_texture_generator.hpp"
#include "klvk/vulkan/texture.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"
#include "verlet/json/json_helpers.hpp"
#include "verlet/json/json_keys.hpp"

namespace verlet
{

VerletApp::VerletApp()
{
    event_listener_ = klvk::events::EventListenerMethodCallbacks<&VerletApp::OnMouseScroll>::CreatePtr(this);
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
    UpdateRenderTransforms();
    UpdateSimulation();
    Render();
}

void VerletApp::InitializeRendering()
{
    SetTargetFramerate({60.f});

    SetClearColor({});
    GetWindow().SetSize(1920, 1080);
    UpdateWorldRange(std::numeric_limits<float>::max());
    GetWindow().SetTitle("Verlet");

    {
        // Generate circle mask texture
        constexpr auto size = Vec2<size_t>{} + 128;
        const auto pixels = klvk::ProceduralTextureGenerator::CircleMask(size, 2);
        texture_ = klvk::Texture::CreateR8(GetDeviceContext(), size.Cast<uint32_t>(), std::span{pixels});
    }

    instance_painter_.Initialize(*this, *texture_);
}

void VerletApp::UpdateWorldRange(float max_extent_change)
{
    const auto half_extent = GetWindow().GetSize2f() / (2 * kPixelsPerWorldUnit);
    world_range_.x = {.begin = -half_extent.x(), .end = half_extent.x()};
    world_range_.y = {.begin = -half_extent.y(), .end = half_extent.y()};

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

    if (max_objects_saturation_)
    {
        max_objects_count_ = static_cast<size_t>(*max_objects_saturation_ * static_cast<float>(ObjectsCapacity()));
    }
}

Vec2f VerletApp::RelativeToWorld(const Vec2f& relative) const
{
    return world_range_.Uniform(.5f) + relative * (world_range_.Extent() / 2);
}

float VerletApp::RelativeToWorldLength(float relative) const
{
    const auto half = world_range_.Extent() / 2;
    return relative * std::min(half.x(), half.y());
}

size_t VerletApp::ObjectsCapacity() const
{
    // Circles of one radius pack in a hexagonal lattice at best, where each takes
    // up a rhombus of this area.
    const float per_object = 2 * std::numbers::sqrt3_v<float> * edt::Math::Sqr(VerletObject::GetRadius());
    const auto area = solver.GetSimArea().Extent();
    return static_cast<size_t>((area.x() * area.y()) / per_object);
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
    klvk::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            static constexpr int indent_size = 4;
            static constexpr char indent_char = ' ';
            nlohmann::json json = JSONHelpers::AppStateToJSON(*this);
            klvk::Filesystem::WriteFile(path, json.dump(indent_size, indent_char));
        });
}

void VerletApp::LoadAppState(const std::filesystem::path& path)
{
    klvk::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            std::string content;
            klvk::Filesystem::ReadFile(path, content);

            const auto json = nlohmann::json::parse(content);

            auto window_size = JSONHelpers::Vec2iFromJSON(json[JSONKeys::kWindowSize]).Cast<uint32_t>();

            // A preset states the budget one way or the other, never both: they
            // would disagree the moment the world was a different size.
            const bool has_count = json.contains(JSONKeys::kMaxObjectsCount);
            const bool has_saturation = json.contains(JSONKeys::kMaxObjectsSaturation);
            klvk::ErrorHandling::Ensure(
                has_count != has_saturation,
                "A preset must contain exactly one of '{}' and '{}'",
                JSONKeys::kMaxObjectsCount,
                JSONKeys::kMaxObjectsSaturation);

            if (has_saturation)
            {
                const float saturation = json[JSONKeys::kMaxObjectsSaturation];
                klvk::ErrorHandling::Ensure(
                    saturation >= 0.f && saturation <= 1.f,
                    "{} must be within [0, 1], got {}",
                    JSONKeys::kMaxObjectsSaturation,
                    saturation);
                max_objects_saturation_ = saturation;
            }
            else
            {
                max_objects_saturation_.reset();
                max_objects_count_ = json[JSONKeys::kMaxObjectsCount];
            }

            GetWindow().SetSize(window_size.x(), window_size.y());

            DeleteAllEmitters();

            // Emitters are stored relative to the world, so a preset needs no
            // adjusting to load into a world of a different size.
            for (const auto& emitter_json : json[JSONKeys::kEmitters])
            {
                AddEmitter(JSONHelpers::EmitterFromJSON(emitter_json));
            }
        });
}

void VerletApp::SavePositions(const std::filesystem::path& path) const
{
    klvk::ErrorHandling::InvokeAndCatchAll(
        [&]
        {
            std::string buffer;
            auto inserter = std::back_inserter(buffer);

            fmt::format_to(inserter, "{}\n", solver.objects.ObjectsCount());
            for (const auto& object : solver.objects.Objects())
            {
                fmt::format_to(inserter, "{} {}\n", object.position.x(), object.position.y());
            }

            klvk::Filesystem::WriteFile(path, buffer);
        });
}

void VerletApp::RenderWorld()
{
    ObjectColorFunction color_function = [](const VerletObject& object)
    {
        return object.color;
    };
    if (tick_color_strategy_) color_function = tick_color_strategy_->GetColorFunction();

    instance_painter_.Clear();

    auto paint_instanced_object = [&](const VerletObject& object) mutable
    {
        const auto& color = color_function(object);
        instance_painter_.DrawObject(object.position, color, object.GetRadius() + Vec2f{});
    };

    perf_stats_.render.total = edt::MeasureTime(
        [&]
        {
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

            instance_painter_.Render(world_to_view_);
        });
}

void VerletApp::OnMouseScroll(const klvk::events::OnMouseScroll& event)
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
    SetClearColor(Vec4f{v.x(), v.y(), v.z(), 1.f});
}

void VerletApp::AddEmitter(std::unique_ptr<Emitter> emitter)
{
    emitters_.push_back(std::move(emitter));
}

Vec2f VerletApp::GetMousePositionInWorldCoordinates() const
{
    const auto screen_size = GetWindow().GetSize2f();
    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const auto mouse_pos = ImGui::GetMousePos();

    Vec2f screen_position{
        (mouse_pos.x - viewport->Pos.x) * io.DisplayFramebufferScale.x,
        (mouse_pos.y - viewport->Pos.y) * io.DisplayFramebufferScale.y};

    screen_position.y() = screen_size.y() - screen_position.y();
    return edt::Math::TransformPos(screen_to_world_, screen_position);
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
