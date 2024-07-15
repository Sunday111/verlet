#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include "EverydayTools/Math/FloatRange.hpp"
#include "EverydayTools/Math/Math.hpp"
#include "instance_painter.hpp"
#include "klgl/application.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/window.hpp"
#include "verlet_solver.hpp"

namespace verlet
{

class Tool;

class VerletApp : public klgl::Application
{
public:
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

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = typename Clock::time_point;
    using Super = klgl::Application;

    VerletApp();
    ~VerletApp() override;

    void Initialize() override;
    void InitializeRendering();
    void Tick() override
    {
        Super::Tick();
        UpdateWorldRange();
        UpdateSimulation();
        Render();
    }

    void UpdateWorldRange();
    void UpdateSimulation();
    void Render();
    void RenderWorld();

    template <typename... Args>
    void GuiText(const fmt::format_string<Args...>& format, Args&&... args)
    {
        std::string_view formatted = FormatTemp(format, std::forward<Args>(args)...);
        ImGui::TextUnformatted(formatted.begin(), formatted.end());
    }

    void RenderGUI();
    void GUI_Tools();

    [[nodiscard]] static constexpr Vec2f TransformPos(const Mat3f& mat, const Vec2f& pos)
    {
        Vec3f v3 = mat.MatMul(Vec3f{{pos.x(), pos.y(), 1.f}});
        return Vec2f{{v3.x(), v3.y()}};
    }

    [[nodiscard]] static constexpr Vec2f TransformVector(const Mat3f& mat, const Vec2f& vec)
    {
        Vec3f v3 = mat.MatMul(Vec3f{{vec.x(), vec.y(), 0.f}});
        return Vec2f{{v3.x(), v3.y()}};
    };

    Vec2f GetMousePositionInWorldCoordinates() const
    {
        const auto window_size = GetWindow().GetSize().Cast<float>();
        const auto window_range = edt::FloatRange2D<float>::FromMinMax({}, window_size);
        const auto window_to_world = edt::Math::MakeTransform(window_range, world_range);

        auto [x, y] = ImGui::GetMousePos();
        y = window_range.y.Extent() - y;
        auto world_pos = TransformPos(window_to_world, Vec2f{x, y});
        return world_pos;
    }

    static constexpr edt::FloatRange<float> kMinSideRange{-100, 100};
    edt::FloatRange2D<float> world_range{.x = {-100.f, 100.f}, .y = {-100.f, 100.f}};
    VerletSolver solver{};

    float last_emit_time = 0.0;

    // Rendering
    std::unique_ptr<klgl::Shader> shader_;

    template <typename... Args>
    std::string_view FormatTemp(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        temp_string_for_formatting_.clear();
        fmt::format_to(std::back_inserter(temp_string_for_formatting_), fmt, std::forward<Args>(args)...);
        return temp_string_for_formatting_;
    }

    std::string temp_string_for_formatting_{};
    InstancedPainter circle_painter_{};

    bool enable_emitter_ = false;

    std::unique_ptr<Tool> tool_;
    PerfStats perf_stats_{};
};

}  // namespace verlet
