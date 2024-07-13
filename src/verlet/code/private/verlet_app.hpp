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

class VerletApp : public klgl::Application
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = typename Clock::time_point;
    using Super = klgl::Application;

    struct HeldObject
    {
        size_t index{};
        bool was_movable{};
    };

    static constexpr edt::FloatRange2D<float> world_range{.x = {-100.f, 100.f}, .y = {-100.f, 100.f}};
    static constexpr Vec2f emitter_pos = world_range.Uniform({0.5, 0.85f});
    static constexpr VerletSolver solver{
        .gravity = Vec2f{0.f, -world_range.y.Extent() / 1.f},
        .constraint_radius = world_range.Extent().x() / 2.f,
    };

    void Initialize() override;
    void InitializeRendering();
    void Tick() override
    {
        Super::Tick();
        UpdateSimulation();
        Render();
    }

    void UpdateSimulation();
    void Render();
    void RenderWorld();
    void RenderGUI();

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

    std::vector<VerletObject> objects;
    std::vector<VerletLink> links;
    float last_emit_time = 0.0;
    bool lmb_hold = false;

    // Rendering
    std::unique_ptr<klgl::Shader> shader_;

    template <typename... Args>
    const char* FormatTemp(const fmt::format_string<Args...> fmt, Args&&... args)
    {
        temp_string_for_formatting_.clear();
        fmt::format_to(std::back_inserter(temp_string_for_formatting_), fmt, std::forward<Args>(args)...);
        return temp_string_for_formatting_.data();
    }

    std::string temp_string_for_formatting_{};
    InstancedPainter circle_painter_{};
    std::chrono::milliseconds last_sim_update_duration_{};

    bool spawn_movable_objects_ = true;
    bool link_spawned_to_previous_ = false;
    bool enable_emitter_ = false;

    std::optional<HeldObject> held_object_;
};

}  // namespace verlet
