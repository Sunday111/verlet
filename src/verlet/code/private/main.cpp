#include <fmt/chrono.h>
#include <fmt/format.h>

#include <ranges>

#include "float_range.hpp"
#include "math.hpp"
#include "matrix.hpp"
#include "measure_time.hpp"
#include "wrap_raylib.hpp"

struct VerletObjects
{
    VerletObjects() = default;
    VerletObjects(const VerletObjects&) = delete;
    VerletObjects(VerletObjects&&) = delete;

    std::vector<Vec2f> position;
    std::vector<Vec2f> old_position;
    std::vector<Vector3<uint8_t>> color;
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
            if (dist_sq > Math::Sqr(min_dist))
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
                if (dist_sq < Math::Sqr(min_dist))
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

int main()
{
    VerletObjects objects;
    objects.position.reserve(3000);
    objects.old_position.reserve(3000);
    objects.color.reserve(3000);
    objects.radius.reserve(3000);

    raylib::InitWindow(1000, 1000, "raylib [core] example - basic window");
    raylib::SetTargetFPS(60);

    const FloatRange2D<float> world_range{.x = {-100.f, 100.f}, .y = {-100.f, 100.f}};
    const Vec2f emitter_pos = world_range.Uniform({0.5, 0.85f});
    const VerletSolver solver{
        .gravity = Vec2f{0.f, -world_range.y.Extent() / 1.f},
        .constraint_radius = world_range.Extent().x() / 2.f,
    };
    float last_emit_time = 0.0;
    float last_duration_print_time = 0.0;

    while (!raylib::WindowShouldClose())
    {
        const FloatRange2D<float> screen_range{
            .x = {0, raylib::GetScreenWidthF()},
            .y = {0, raylib::GetScreenHeightF()}};
        const Mat3f world_to_screen = Math::MakeTransform(world_range, screen_range);
        [[maybe_unused]] const Mat3f screen_to_world = Math::MakeTransform(screen_range, world_range);

        auto transform_pos = [](const Mat3f& mat, const Vec2f& pos)
        {
            Vec3f v3 = mat.MatMul(Vec3f{{pos.x(), pos.y(), 1.f}});
            return Vec2f{{v3.x(), v3.y()}};
        };

        auto transform_vector = [](const Mat3f& mat, const Vec2f& vec)
        {
            Vec3f v3 = mat.MatMul(Vec3f{{vec.x(), vec.y(), 0.f}});
            return Vec2f{{v3.x(), v3.y()}};
        };

        auto to_screen_coord = [&](const Vec2f& plot_pos)
        {
            Vec2f ipos = transform_pos(world_to_screen, plot_pos);
            ipos.y() = screen_range.y.Extent() - ipos.y();
            return ipos;
        };

        auto get_world_mouse_pos = [&]()
        {
            auto [x, y] = raylib::GetMousePos();
            y = screen_range.y.Extent() - y;
            auto world_pos = transform_pos(screen_to_world, Vec2f{x, y});
            return world_pos;
        };

        const float time = static_cast<float>(raylib::GetTime());
        const float dt = raylib::GetFrameTime();
        auto spawn_at = [&](const Vec2f& position, const Vec2f& velocity, const float radius)
        {
            const size_t index = objects.Add();
            objects.position[index] = position;
            objects.old_position[index] = solver.MakePreviousPosition(position, velocity, dt);
            objects.color[index] = Math::GetRainbowColors(time);
            objects.radius[index] = radius;
        };

        if (raylib::IsMouseButtonPressed(raylib::MouseButton::Left))
        {
            spawn_at(get_world_mouse_pos(), {0.f, 0.f}, 1.f);
        }

        // Emitter
        if (time - last_emit_time > 0.05f)
        {
            last_emit_time = time;
            constexpr float velocity_mag = 0.015f;
            constexpr float emitter_rotation_speed = 4.0f;
            const Vec2f direction{std::cos(emitter_rotation_speed * time), std::sin(emitter_rotation_speed * time)};
            spawn_at(emitter_pos, direction * velocity_mag, 1.f);
        }

        const auto [update_duration] = MeasureTime<std::chrono::milliseconds>(
            [&]
            {
                solver.Update(objects, dt);
            });

        raylib::BeginDrawing();
        raylib::ClearBackground(255, 245, 153);

        const auto [render_duration] = MeasureTime<std::chrono::milliseconds>(
            [&]
            {
                // Draw constraint
                {
                    const auto radius = transform_vector(world_to_screen, {solver.constraint_radius, 0.0f}).x();
                    const Vec2f position = screen_range.Extent() / 2;
                    raylib::DrawCircle({position.x(), position.y()}, radius, raylib::kBlack);
                }

                for (const size_t index : std::views::iota(0uz, objects.Size()))
                {
                    const auto screen_pos = to_screen_coord(objects.position[index]);
                    const auto screen_size = transform_vector(world_to_screen, {objects.radius[index], 0.0f});
                    const auto& color = objects.color[index];
                    raylib::DrawCircle(
                        {screen_pos.x(), screen_pos.y()},
                        screen_size.x(),
                        {color.x(), color.y(), color.z(), 255});
                }

                raylib::EndDrawing();
            });

        if (time - last_duration_print_time > 1.f)
        {
            last_duration_print_time = time;
            fmt::print("Perf stats:\n");
            fmt::print("    Objects count: {}\n", objects.Size());
            fmt::print("    Solver update duration: {}\n", update_duration);
            fmt::print("    Render duration: {}\n", render_duration);
            fmt::println("");
        }
    }

    raylib::CloseWindow();
}
