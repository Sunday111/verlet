#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include "camera.hpp"
#include "edt/math/float_range.hpp"
#include "emitters/emitter.hpp"
#include "instance_painter.hpp"
#include "klvk/application.hpp"
#include "klvk/window.hpp"
#include "physics/verlet_solver.hpp"

namespace klvk
{
class Texture;
}  // namespace klvk

namespace klvk::events
{
class IEventListener;
class OnMouseScroll;
};  // namespace klvk::events

namespace verlet
{

class Tool;
class SpawnColorStrategy;
class TickColorStrategy;
class Emitter;

class VerletApp : public klvk::Application
{
public:
    using Super = klvk::Application;

    struct RenderPerfStats
    {
        std::chrono::nanoseconds total;
        std::chrono::nanoseconds set_circle_loop;
    };

    struct PerfStats
    {
        VerletSolver::UpdateStats sim_update;
        RenderPerfStats render;
    };

    // Screen size decides how much world there is: an object covers the same
    // number of pixels at every resolution, and a bigger window simulates a
    // bigger world rather than the same world drawn larger.
    static constexpr float kPixelsPerWorldUnit = 3.6f;

    VerletApp();
    ~VerletApp() override;

    void Initialize() override;
    void InitializeRendering();
    void Tick() override;

    void UpdateWorldRange(float max_extent_change = 0.5f);
    void UpdateCamera();
    void UpdateTools();
    void UpdateStepping();
    void UpdateSimulation();
    void Render();

    // Only the simulation pauses. Rendering, the camera and the tools keep
    // running, so a frozen pile can still be looked at and painted into.
    [[nodiscard]] bool IsPaused() const noexcept { return paused_; }
    void SetPaused(bool paused) noexcept { paused_ = paused; }

    // Advances one step and stays paused, which is the only way to ask for a
    // step: running is what the play button is for.
    void RequestStep() noexcept
    {
        paused_ = true;
        step_requested_ = true;
    }

    void UpdateRenderTransforms();
    void RenderWorld();

    void SaveAppState(const std::filesystem::path& path) const;
    void LoadAppState(const std::filesystem::path& path);
    void SavePositions(const std::filesystem::path& path) const;

    void OnMouseScroll(const klvk::events::OnMouseScroll&);

    [[nodiscard]] const PerfStats& GetPerfStats() const { return perf_stats_; }

    [[nodiscard]] auto GetEmitters() const
    {
        return emitters_ | std::views::transform([](const auto& ptr) -> auto& { return *ptr; });
    }
    [[nodiscard]] Camera& GetCamera() { return camera_; }
    [[nodiscard]] const Camera& GetCamera() const { return camera_; }
    [[nodiscard]] const edt::FloatRange2Df& GetWorldRange() const { return world_range_; }
    [[nodiscard]] const edt::Vec3f& GetBackgroundColor() const { return background_color_; }
    [[nodiscard]] const Mat3f& GetWorldToViewTransform() const { return world_to_view_; }
    void SetBackgroundColor(const Vec3f& background_color);
    void AddEmitter(std::unique_ptr<Emitter> emitter);

    [[nodiscard]] Vec2f GetMousePositionInWorldCoordinates() const;
    InstancedPainter& GetPainter() { return instance_painter_; }

    // How many objects the world holds when they are packed as tightly as circles
    // go. The budget can be stated as a share of this instead of as a count, which
    // is what makes it mean the same thing at any resolution.
    [[nodiscard]] size_t ObjectsCapacity() const;

    // Emitters are placed in relative coordinates: -1 and 1 are the edges of the
    // world on each axis and the origin is its centre, so a preset says where a
    // thing is without knowing how big the world turned out to be.
    [[nodiscard]] Vec2f RelativeToWorld(const Vec2f& relative) const;

    // A relative length becomes a world one against the shorter half of the
    // world, so a circle stays a circle whatever the aspect ratio.
    [[nodiscard]] float RelativeToWorldLength(float relative) const;

    // Effective budget. Recomputed from the saturation whenever the world changes,
    // so this is the one emitters and the GUI read either way.
    size_t max_objects_count_ = 10000;

    // When set, the budget is this share of ObjectsCapacity() rather than the
    // count above, and a preset carrying it survives a change of resolution.
    std::optional<float> max_objects_saturation_;
    size_t time_steps_ = 0;
    bool paused_ = false;
    bool step_requested_ = false;

    VerletSolver solver{};
    std::unique_ptr<Tool> tool_;
    std::unique_ptr<SpawnColorStrategy> spawn_color_strategy_;
    std::unique_ptr<TickColorStrategy> tick_color_strategy_;

    void DeleteAllEmitters();
    void EnableAllEmitters();
    void DisableAllEmitters();

private:
    std::unique_ptr<klvk::events::IEventListener> event_listener_;

    edt::FloatRange2D<float> world_range_{};

    std::unique_ptr<klvk::Texture> texture_;

    Camera camera_{};
    InstancedPainter instance_painter_{};
    std::vector<std::unique_ptr<Emitter>> emitters_{};
    PerfStats perf_stats_{};
    Vec3f background_color_{};

    Mat3f world_to_camera_;
    Mat3f world_to_view_;

    Mat3f screen_to_world_;
};

}  // namespace verlet
