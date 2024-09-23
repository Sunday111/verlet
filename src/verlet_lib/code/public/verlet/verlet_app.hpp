#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include "EverydayTools/Math/FloatRange.hpp"
#include "camera.hpp"
#include "emitters/emitter.hpp"
#include "instance_painter.hpp"
#include "klgl/application.hpp"
#include "klgl/shader/uniform_handle.hpp"
#include "klgl/window.hpp"
#include "physics/verlet_solver.hpp"

namespace klgl
{
class Shader;
class Texture;
}  // namespace klgl

namespace klgl::events
{
class IEventListener;
class OnMouseScroll;
};  // namespace klgl::events

namespace verlet
{

class Tool;
class SpawnColorStrategy;
class TickColorStrategy;
class Emitter;

class VerletApp : public klgl::Application
{
public:
    using Super = klgl::Application;

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

    static constexpr edt::FloatRange<float> kMinSideRange{-150, 150};

    VerletApp();
    ~VerletApp() override;

    void Initialize() override;
    void InitializeRendering();
    void Tick() override;

    void UpdateWorldRange(float max_extent_change = 0.5f);
    void UpdateCamera();
    void UpdateSimulation();
    void Render();

    void UpdateRenderTransforms();
    void RenderWorld();

    void SaveAppState(const std::filesystem::path& path) const;
    void LoadAppState(const std::filesystem::path& path);
    void SavePositions(const std::filesystem::path& path) const;

    void OnMouseScroll(const klgl::events::OnMouseScroll&);

    [[nodiscard]] const PerfStats& GetPerfStats() const { return perf_stats_; }

    template <typename Self>
    [[nodiscard]] auto GetEmitters(this Self& self)
    {
        return self.emitters_ | std::views::transform([](const auto& ptr) -> auto& { return *ptr; });
    }
    [[nodiscard]] Camera& GetCamera() { return camera_; }
    [[nodiscard]] const Camera& GetCamera() const { return camera_; }
    [[nodiscard]] const edt::FloatRange2Df& GetWorldRange() const { return world_range_; }
    [[nodiscard]] const edt::Vec3f& GetBackgroundColor() const { return background_color_; }
    [[nodiscard]] const Mat3f& GetWorldToViewTransform() const { return world_to_view_; }
    void SetBackgroundColor(const Vec3f& background_color);
    void AddEmitter(std::unique_ptr<Emitter> emitter);

    Vec2f GetMousePositionInWorldCoordinates() const;
    InstancedPainter& GetPainter() { return instance_painter_; }

    size_t max_objects_count_ = 10000;
    size_t time_steps_ = 0;

    VerletSolver solver{};
    std::unique_ptr<Tool> tool_;
    std::unique_ptr<SpawnColorStrategy> spawn_color_strategy_;
    std::unique_ptr<TickColorStrategy> tick_color_strategy_;

private:
    std::unique_ptr<klgl::events::IEventListener> event_listener_;

    edt::FloatRange2D<float> world_range_{.x = kMinSideRange, .y = kMinSideRange};

    klgl::UniformHandle u_world_to_view_ = klgl::UniformHandle("u_world_to_view");
    std::unique_ptr<klgl::Shader> shader_;
    std::unique_ptr<klgl::Texture> texture_;

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
