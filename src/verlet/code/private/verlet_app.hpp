#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include "EverydayTools/Math/FloatRange.hpp"
#include "instance_painter.hpp"
#include "klgl/application.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/window.hpp"
#include "physics/verlet_solver.hpp"

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

class VerletEmitter
{
public:
    float last_emit_time = 0.0;
    bool enabled = false;
    size_t max_objects_count = 10000;
};

class Camera
{
public:
    float zoom = 1.f;
    Vec2f eye{};
};

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

    static constexpr edt::FloatRange<float> kMinSideRange{-100, 100};

    VerletApp();
    ~VerletApp() override;

    void Initialize() override;
    void InitializeRendering();
    void Tick() override;

    void UpdateWorldRange();
    void UpdateCamera();
    void UpdateSimulation();
    void Render();
    void RenderWorld();

    void OnMouseScroll(const klgl::events::OnMouseScroll&);

    [[nodiscard]] static constexpr Vec2f TransformPos(const Mat3f& mat, const Vec2f& pos)
    {
        Vec3f v3 = mat.MatMul(Vec3f{{pos.x(), pos.y(), 1.f}});
        return Vec2f{{v3.x(), v3.y()}};
    }

    [[nodiscard]] static constexpr Vec2f TransformVector(const Mat3f& mat, const Vec2f& vec)
    {
        Vec3f v3 = mat.MatMul(Vec3f{{vec.x(), vec.y(), 0.f}});
        return Vec2f{{v3.x(), v3.y()}};
    }

    [[nodiscard]] const PerfStats& GetPerfStats() const { return perf_stats_; }

    [[nodiscard]] VerletEmitter& GetEmitter() { return emitter_; }
    [[nodiscard]] const VerletEmitter& GetEmitter() const { return emitter_; }
    [[nodiscard]] Camera& GetCamera() { return camera_; }
    [[nodiscard]] const Camera& GetCamera() const { return camera_; }
    [[nodiscard]] const edt::FloatRange2Df& GetWorldRange() const { return world_range_; }
    [[nodiscard]] const edt::Vec3f& GetBackgroundColor() const { return background_color_; }
    void SetBackgroundColor(const Vec3f& background_color);

    Vec2f GetMousePositionInWorldCoordinates() const;

    VerletSolver solver{};
    std::unique_ptr<Tool> tool_;
    std::unique_ptr<SpawnColorStrategy> spawn_color_strategy_;
    std::unique_ptr<TickColorStrategy> tick_color_strategy_;

private:
    std::unique_ptr<klgl::events::IEventListener> event_listener_;
    edt::FloatRange2D<float> world_range_{.x = kMinSideRange, .y = kMinSideRange};
    std::unique_ptr<klgl::Shader> shader_;
    Camera camera_{};
    InstancedPainter circle_painter_{};
    VerletEmitter emitter_{};
    PerfStats perf_stats_{};
    Vec3f background_color_{};
};

}  // namespace verlet
