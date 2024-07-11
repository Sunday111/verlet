#include <fmt/chrono.h>
#include <fmt/format.h>
#include <imgui.h>

#include <ranges>
#include <utility>

#include "EverydayTools/Math/FloatRange.hpp"
#include "EverydayTools/Math/Math.hpp"
#include "instance_painter.hpp"
#include "klgl/application.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/window.hpp"
#include "measure_time.hpp"

namespace verlet
{

struct VerletObjects
{
    VerletObjects() = default;
    VerletObjects(const VerletObjects&) = delete;
    VerletObjects(VerletObjects&&) = delete;

    std::vector<Vec2f> position;
    std::vector<Vec2f> old_position;
    std::vector<Vec3<uint8_t>> color;
    std::vector<float> radius;

    [[nodiscard]] size_t Add()
    {
        size_t index = Size();
        position.emplace_back(Vec2f{});
        old_position.emplace_back(Vec2f{});
        color.push_back({{255, 0, 0}});
        radius.push_back(1.f);
        return index;
    }

    [[nodiscard]] size_t Size() const
    {
        return position.size();
    }

    [[nodiscard]] auto Indices() const
    {
        return std::views::iota(0uz, Size());
    }
};

struct VerletSolver
{
    Vec2f gravity{0.0f, -0.75f};
    float constraint_radius = 1.f;
    size_t sub_steps = 8;
    float collision_response = 0.75f;

    void Update(VerletObjects& objects, const float dt) const
    {
        const float sub_dt = dt / static_cast<float>(sub_steps);
        for ([[maybe_unused]] const size_t index : std::views::iota(0uz, sub_steps))
        {
            ApplyConstraint(objects);
            SolveCollisions(objects);
            UpdatePosition(objects, sub_dt);
        }
    }

    void UpdatePosition(VerletObjects& objects, const float dt) const
    {
        for (const size_t index : objects.Indices())
        {
            const Vec2f velocity = objects.position[index] - objects.old_position[index];

            // Save current position
            objects.old_position[index] = objects.position[index];

            // Perform Verlet integration
            objects.position[index] += velocity + gravity * dt * dt;
        }
    }

    void ApplyConstraint(VerletObjects& objects) const
    {
        for (const size_t index : objects.Indices())
        {
            const float min_dist = constraint_radius - objects.radius[index];
            const float dist_sq = objects.position[index].SquaredLength();
            if (dist_sq > edt::Math::Sqr(min_dist))
            {
                const float dist = std::sqrt(dist_sq);
                const Vec2f direction = objects.position[index] / dist;
                objects.position[index] = direction * (constraint_radius - objects.radius[index]);
            }
        }
    }

    void SolveCollisions(VerletObjects& objects) const
    {
        const size_t objects_count = objects.Size();
        for (const size_t i : std::views::iota(0uz, objects_count))
        {
            const auto a_r = objects.radius[i];
            auto& a_pos = objects.position[i];
            for (const size_t j : std::views::iota(i + 1, objects_count))
            {
                const auto b_r = objects.radius[j];
                auto& b_pos = objects.position[j];
                const float min_dist = a_r + b_r;
                const Vec2f rel = a_pos - b_pos;
                const float dist_sq = rel.SquaredLength();
                if (dist_sq < edt::Math::Sqr(min_dist))
                {
                    const float dist = std::sqrt(dist_sq);
                    const Vec2f dir = rel / dist;
                    const float delta = 0.5f * collision_response * (min_dist - dist);

                    const float ka = a_r / min_dist;
                    a_pos += ka * delta * dir;

                    const float kb = b_r / min_dist;
                    b_pos -= kb * delta * dir;
                }
            }
        }
    }

    [[nodiscard]] Vec2f MakePreviousPosition(const Vec2f& current_position, const Vec2f& velocity, const float dt) const
    {
        return current_position - velocity / (dt * static_cast<float>(sub_steps));
    }
};

class VerletApp : public klgl::Application
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = typename Clock::time_point;
    using Super = klgl::Application;

    void Initialize() override;
    void InitializeRendering();
    void Tick() override
    {
        Super::Tick();
        UpdateSimulation();
        Render();
    }
    void PostTick() override;

    static constexpr edt::FloatRange2D<float> world_range{.x = {-100.f, 100.f}, .y = {-100.f, 100.f}};
    static constexpr Vec2f emitter_pos = world_range.Uniform({0.5, 0.85f});
    static constexpr VerletSolver solver{
        .gravity = Vec2f{0.f, -world_range.y.Extent() / 1.f},
        .constraint_radius = world_range.Extent().x() / 2.f,
    };

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

    VerletObjects objects;
    float last_emit_time = 0.0;
    TimePoint start_time;

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
};

void VerletApp::Initialize()
{
    Super::Initialize();
    InitializeRendering();

    start_time = Clock::now();

    objects.position.reserve(3000);
    objects.old_position.reserve(3000);
    objects.color.reserve(3000);
    objects.radius.reserve(3000);
}

void VerletApp::InitializeRendering()
{
    klgl::OpenGl::SetClearColor({255, 245, 153, 255});
    GetWindow().SetSize(1000, 1000);
    GetWindow().SetTitle("Verlet");

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    klgl::Shader::shaders_dir_ = shaders_dir;

    shader_ = std::make_unique<klgl::Shader>("just_color.shader.json");
    shader_->Use();

    circle_painter_.Initialize();
}

void VerletApp::UpdateSimulation()
{
    const float dt = GetLastFrameDurationSeconds();
    const TimePoint current_time = Clock::now();
    const float relative_time =
        std::chrono::duration_cast<std::chrono::duration<float>>(current_time - start_time).count();

    auto spawn_at = [&](const Vec2f& position, const Vec2f& velocity, const float radius)
    {
        const size_t index = objects.Add();
        objects.position[index] = position;
        objects.old_position[index] = solver.MakePreviousPosition(position, velocity, dt);
        objects.color[index] = edt::Math::GetRainbowColors(relative_time);
        objects.radius[index] = radius;
    };

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        spawn_at(GetMousePositionInWorldCoordinates(), {0.f, 0.f}, 1.f);
    }

    // Emitter
    if (relative_time - last_emit_time > 0.1f)
    {
        last_emit_time = relative_time;
        constexpr float velocity_mag = 0.001f;
        constexpr float emitter_rotation_speed = 3.0f;
        const Vec2f direction{
            std::cos(emitter_rotation_speed * relative_time),
            std::sin(emitter_rotation_speed * relative_time)};
        spawn_at(emitter_pos, direction * velocity_mag, 2.f + std::sin(3 * relative_time));
    }

    const auto [update_duration] = MeasureTime<std::chrono::milliseconds>(
        [&]
        {
            solver.Update(objects, dt);
        });
    last_sim_update_duration_ = update_duration;
}

void VerletApp::PostTick()
{
    Super::PostTick();
}

void VerletApp::Render()
{
    RenderWorld();
    RenderGUI();
}

void VerletApp::RenderWorld()
{
    const klgl::ScopeAnnotation annotation("Render World");

    klgl::OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const edt::FloatRange2D<float> screen_range{.x = {-1, 1}, .y = {-1, 1}};
    const Mat3f world_to_screen = edt::Math::MakeTransform(world_range, screen_range);

    [[maybe_unused]] auto to_screen_coord = [&](const Vec2f& plot_pos)
    {
        return TransformPos(world_to_screen, plot_pos);
    };

    size_t circle_index = 0;

    // Draw constraint
    circle_painter_.SetCircle(circle_index++, Vec2f{}, Vec3f{}, 2 * solver.constraint_radius / world_range.Extent());

    for (const size_t index : objects.Indices())
    {
        const auto screen_pos = to_screen_coord(objects.position[index]);
        const auto screen_size = TransformVector(world_to_screen, objects.radius[index] + Vec2f{});
        const auto& color = objects.color[index];

        circle_painter_.SetCircle(circle_index++, screen_pos, color.Cast<float>() / 255, screen_size);
    }

    shader_->Use();
    circle_painter_.Render();
}

void VerletApp::RenderGUI()
{
    const klgl::ScopeAnnotation annotation("Render GUI");
    ImGui::Begin("Parameters");
    shader_->DrawDetails();
    ImGui::Text("%s", FormatTemp("Framerate: {}", GetFramerate()));                      // NOLINT
    ImGui::Text("%s", FormatTemp("Objects count: {}", objects.Size()));                  // NOLINT
    ImGui::Text("%s", FormatTemp("Sim update duration {}", last_sim_update_duration_));  // NOLINT
    ImGui::End();
}

}  // namespace verlet

int main()
{
    verlet::VerletApp app;
    app.Run();
    return 0;
}
