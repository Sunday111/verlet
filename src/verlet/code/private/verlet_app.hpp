#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include "EverydayTools/Math/FloatRange.hpp"
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

class VerletEmitter
{
public:
    float last_emit_time = 0.0;
    bool enabled = false;
    size_t max_objects_count = 10000;
};

template <typename T>
struct ValueAnimation
{
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    // Returns true if animation is completed
    bool Update(T& value)
    {
        const auto now = Clock::now();
        if (now > begin_time + std::chrono::duration<float>(duration_seconds))
        {
            value = final_value;
            return true;
        }

        const auto dt =
            std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - begin_time).count();
        const float t = std::clamp(dt / duration_seconds, 0.f, 1.f);
        value = start_value + t * (final_value - start_value);

        return false;
    }

    T start_value{};
    T final_value{};
    float duration_seconds = 0.f;

    TimePoint begin_time = Clock::now();
};

class Camera
{
public:
    [[nodiscard]] const edt::FloatRange2Df& GetRange() const { return range_; }
    [[nodiscard]] float GetZoom() const { return zoom_; }
    [[nodiscard]] const Vec2f& GetEye() const { return eye_; }

    void Zoom(const float delta);
    void Pan(const Vec2f& delta);

    void Update(const edt::FloatRange2Df& world_range);

    float zoom_speed = 0.2f;
    float zoom_animation_diration_seconds = 0.3f;
    float pan_speed = 0.3f;
    float pan_animation_diration_seconds = 0.3f;
    bool animate = true;

private:
    edt::FloatRange2Df ComputeRange(const edt::FloatRange2Df& world_range) const;

private:
    edt::FloatRange2Df range_ = {};

    float zoom_ = 1.f;
    std::optional<ValueAnimation<float>> zoom_animation_;

    Vec2f eye_{};
    std::optional<ValueAnimation<Vec2f>> eye_animation_;
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

    void UpdateRenderTransforms();
    void RenderWorld();

    void OnMouseScroll(const klgl::events::OnMouseScroll&);

    [[nodiscard]] const PerfStats& GetPerfStats() const { return perf_stats_; }

    [[nodiscard]] VerletEmitter& GetEmitter() { return emitter_; }
    [[nodiscard]] const VerletEmitter& GetEmitter() const { return emitter_; }
    [[nodiscard]] Camera& GetCamera() { return camera_; }
    [[nodiscard]] const Camera& GetCamera() const { return camera_; }
    [[nodiscard]] const edt::FloatRange2Df& GetWorldRange() const { return world_range_; }
    [[nodiscard]] const edt::Vec3f& GetBackgroundColor() const { return background_color_; }
    [[nodiscard]] const Mat3f& GetWorldToViewTransform() const { return world_to_view_; }
    void SetBackgroundColor(const Vec3f& background_color);

    Vec2f GetMousePositionInWorldCoordinates() const;
    InstancedPainter& GetPainter() { return instance_painter_; }

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
    VerletEmitter emitter_{};
    PerfStats perf_stats_{};
    Vec3f background_color_{};

    Mat3f world_to_camera_;
    Mat3f world_to_view_;

    Mat3f screen_to_world_;
};

}  // namespace verlet
