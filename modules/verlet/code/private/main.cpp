#include <fmt/chrono.h>
#include <fmt/format.h>

#include <Eigen/Dense>
#include <concepts>
#include <ranges>
#include <span>

#include "measure_time.hpp"
#include "raylib.h"

// Next position = Position + Velocity * dt
// Velocity is deduced from the previous time step

using Vec2f = Eigen::Vector2f;
using Vec2i = Eigen::Vector2i;
using Vec3f = Eigen::Vector3f;
using Mat3f = Eigen::Matrix3f;

// returns true if begin <= x < end, i.e x in [begin; end)
template <std::integral T>
[[nodiscard]] constexpr bool InRange(const T& x, const T& begin, const T& end)
{
    return x >= begin and x < end;
}

template <std::integral T>
class IntRange
{
public:
    [[nodiscard]] constexpr bool Contains(const T& value) const noexcept
    {
        return InRange(value, begin, end);
    }

    T begin = std::numeric_limits<T>::lowest();
    T end = std::numeric_limits<T>::max();
};

template <std::integral T>
class IntRange2D
{
public:
    [[nodiscard]] constexpr bool Contains(const std::tuple<T, T>& p) const noexcept
    {
        return x.Contains(std::get<0>(p)) && y.Contains(std::get<1>(p));
    }
    [[nodiscard]] constexpr bool Contains(const T& vx, const T& vy) const noexcept
    {
        return x.Contains(vx) && y.Contains(vy);
    }

    IntRange<T> x{};
    IntRange<T> y{};
};

template <std::floating_point T>
class FloatRange
{
public:
    [[nodiscard]] constexpr T Extent() const
    {
        return end - begin;
    }

    T begin = std::numeric_limits<T>::lowest();
    T end = std::numeric_limits<T>::max();
};

template <std::floating_point T>
class FloatRange2D
{
public:
    [[nodiscard]] constexpr bool Contains(const std::tuple<T, T>& p) const noexcept
    {
        return x.Contains(std::get<0>(p)) && y.Contains(std::get<1>(p));
    }
    [[nodiscard]] constexpr bool Contains(const T& vx, const T& vy) const noexcept
    {
        return x.Contains(vx) && y.Contains(vy);
    }
    [[nodiscard]] constexpr Vec2f Extent() const noexcept
    {
        return Vec2f{x.Extent(), y.Extent()};
    }

    [[nodiscard]] constexpr Vec2f Min() const noexcept
    {
        return {x.begin, y.begin};
    }

    [[nodiscard]] constexpr Vec2f Uniform(const float v) const noexcept
    {
        return Uniform(Vec2f{v, v});
    }

    [[nodiscard]] constexpr Vec2f Uniform(const Vec2f& v) const noexcept
    {
        return Min() + Vec2f(v.array().cwiseProduct(Extent().array()));
    }

    FloatRange<T> x{};
    FloatRange<T> y{};
};

struct VerletObjects
{
    VerletObjects() = default;
    VerletObjects(const VerletObjects&) = delete;
    VerletObjects(VerletObjects&&) = delete;

    std::vector<Vec2f> position;
    std::vector<Vec2f> old_position;
    std::vector<Eigen::Vector3<uint8_t>> color;
    std::vector<float> radius;

    void UpdatePosition(const size_t index, float dt, const Vec2f& acceleration)
    {
        const Vec2f velocity = position[index] - old_position[index];

        // Save current position
        old_position[index] = position[index];

        // Perform Verlet integration
        position[index] += velocity + acceleration * dt * dt;
    }

    [[nodiscard]] size_t Add()
    {
        size_t index = Size();
        position.emplace_back(Vec2f::Zero());
        old_position.emplace_back(Vec2f::Zero());
        color.push_back({255, 0, 0});
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

template <typename T>
[[nodiscard]] inline constexpr T Sqr(T x)
{
    return x * x;
}

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
            objects.UpdatePosition(index, dt, gravity);
        }
    }

    void ApplyConstraint(VerletObjects& objects) const
    {
        for (const size_t index : objects.Indices())
        {
            const float min_dist = constraint_radius - objects.radius[index];
            const float dist_sq = objects.position[index].squaredNorm();
            if (dist_sq > Sqr(min_dist))
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
                const float dist_sq = rel.squaredNorm();
                if (dist_sq < Sqr(min_dist))
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

Mat3f MakeTransform(const FloatRange2D<float>& from, const FloatRange2D<float>& to)
{
    Mat3f uniform_to_canvas = Mat3f::Identity();
    const float tx = -from.x.begin;
    const float ty = -from.y.begin;
    const float sx = to.x.Extent() / from.x.Extent();
    const float sy = to.y.Extent() / from.y.Extent();
    uniform_to_canvas.coeffRef(0, 0) = sx;
    uniform_to_canvas.coeffRef(1, 1) = sy;
    uniform_to_canvas.coeffRef(0, 2) = tx * sx + to.x.begin;
    uniform_to_canvas.coeffRef(1, 2) = ty * sy + to.y.begin;
    return uniform_to_canvas;
}

template <std::floating_point T>
inline constexpr T kPI = static_cast<T>(M_PI);

[[nodiscard]] constexpr Eigen::Vector3<uint8_t> GetRainbow(const float t)
{
    const float r = std::sin(t);
    const float g = std::sin(t + 0.33f * 2.0f * kPI<float>);
    const float b = std::sin(t + 0.66f * 2.0f * kPI<float>);
    return {
        static_cast<uint8_t>(255.0f * r * r),
        static_cast<uint8_t>(255.0f * g * g),
        static_cast<uint8_t>(255.0f * b * b)};
}

int main()
{
    VerletObjects objects;
    objects.position.reserve(3000);
    objects.old_position.reserve(3000);
    objects.color.reserve(3000);
    objects.radius.reserve(3000);

    InitWindow(1000, 1000, "raylib [core] example - basic window");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    const FloatRange2D<float> world_range{.x = {-100.f, 100.f}, .y = {-100.f, 100.f}};
    const Vec2f emitter_pos = world_range.Uniform({0.5, 0.85});
    const VerletSolver solver{
        .gravity = Vec2f{0.f, -world_range.y.Extent() / 1.f},
        .constraint_radius = world_range.Extent().x() / 2.f,
    };
    float last_emit_time = 0.0;
    float last_duration_print_time = 0.0;

    while (!WindowShouldClose())
    {
        const FloatRange2D<float> screen_range{
            .x = {0, static_cast<float>(GetScreenWidth())},
            .y = {0, static_cast<float>(GetScreenHeight())}};
        const Mat3f world_to_screen = MakeTransform(world_range, screen_range);
        [[maybe_unused]] const Mat3f screen_to_world = MakeTransform(screen_range, world_range);

        auto transform_pos = [](const Mat3f& mat, const Vec2f& pos)
        {
            Vec3f v3 = mat * Vec3f(pos.x(), pos.y(), 1.f);
            return Vec2f{v3.x(), v3.y()};
        };

        auto transform_vector = [](const Mat3f& mat, const Vec2f& vec)
        {
            Vec3f v3 = mat * Vec3f(vec.x(), vec.y(), 0.f);
            return Vec2f{v3.x(), v3.y()};
        };

        auto to_screen_coord = [&](const Vec2f& plot_pos)
        {
            Vec2f ipos = transform_pos(world_to_screen, plot_pos);
            ipos.y() = screen_range.y.Extent() - ipos.y();
            return ipos;
        };

        auto get_world_mouse_pos = [&]()
        {
            auto [x, y] = GetMousePosition();
            y = screen_range.y.Extent() - y;
            auto world_pos = transform_pos(screen_to_world, Vec2f{x, y});
            return world_pos;
        };

        const float time = static_cast<float>(GetTime());
        const float dt = GetFrameTime();
        auto spawn_at = [&](const Vec2f& position, const Vec2f& velocity, const float radius)
        {
            const size_t index = objects.Add();
            objects.position[index] = position;
            objects.old_position[index] = solver.MakePreviousPosition(position, velocity, dt);
            objects.color[index] = GetRainbow(time);
            objects.radius[index] = radius;
        };

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
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

        BeginDrawing();
        ClearBackground({255, 245, 153, 255});

        const auto [render_duration] = MeasureTime<std::chrono::milliseconds>(
            [&]
            {
                // Draw constraint
                {
                    const auto radius = transform_vector(world_to_screen, {solver.constraint_radius, 0.0f}).x();
                    const Vec2f position = screen_range.Extent() / 2;
                    DrawCircleV({position.x(), position.y()}, radius, BLACK);
                }

                for (const size_t index : std::views::iota(0uz, objects.Size()))
                {
                    const auto screen_pos = to_screen_coord(objects.position[index]);
                    const auto screen_size = transform_vector(world_to_screen, {objects.radius[index], 0.0f});
                    const auto& color = objects.color[index];
                    DrawCircleV(
                        {screen_pos.x(), screen_pos.y()},
                        screen_size.x(),
                        Color{color.x(), color.y(), color.z(), 255});
                }

                EndDrawing();
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

    CloseWindow();
}
