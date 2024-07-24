#pragma once

#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

template <typename T, auto extent = std::dynamic_extent>
[[nodiscard]] constexpr inline auto IndicesView(std::span<T, extent> span)
{
    return std::views::iota(size_t{0}, span.size());
}

struct VerletObject
{
    Vec2f position{};
    Vec2f old_position{};
    Vec3<uint8_t> color;
    float radius = 1.f;

    bool movable = true;
};

struct VerletLink
{
    float target_distance{};
    size_t first{};
    size_t second{};
};

struct VerletSolver
{
    edt::Vec2f gravity{0.0f, -0.75f};
    float constraint_radius = 1.f;
    size_t sub_steps = 8;
    float collision_response = 0.75f;

    void Update(std::span<VerletObject> objects, std::span<VerletLink> links, const float dt) const
    {
        const float sub_dt = dt / static_cast<float>(sub_steps);
        for ([[maybe_unused]] const size_t index : std::views::iota(size_t{0}, sub_steps))
        {
            ApplyConstraint(objects);
            ApplyLinks(objects, links);
            SolveCollisions(objects);
            UpdatePosition(objects, sub_dt);
        }
    }

    void UpdatePosition(std::span<VerletObject> objects, const float dt) const
    {
        for (auto& object : objects)
        {
            [[likely]] if (object.movable)
            {
                const auto velocity = object.position - object.old_position;

                // Save current position
                object.old_position = object.position;

                // Perform Verlet integration
                object.position += velocity + gravity * dt * dt;
            }
        }
    }

    void ApplyConstraint(std::span<VerletObject> objects) const
    {
        for (auto& object : objects)
        {
            if (object.movable)
            {
                const float min_dist = constraint_radius - object.radius;
                const float dist_sq = object.position.SquaredLength();
                if (dist_sq > edt::Math::Sqr(min_dist))
                {
                    const float dist = std::sqrt(dist_sq);
                    const auto direction = object.position / dist;
                    object.position = direction * (constraint_radius - object.radius);
                }
            }
        }
    }

    void SolveCollisions(std::span<VerletObject> objects) const
    {
        const size_t objects_count = objects.size();
        for (const size_t i : IndicesView(objects))
        {
            auto& a = objects[i];
            for (const size_t j : std::views::iota(i + 1, objects_count))
            {
                auto& b = objects[j];
                const float min_dist = a.radius + b.radius;
                const auto rel = a.position - b.position;
                const float dist_sq = rel.SquaredLength();
                if (dist_sq < edt::Math::Sqr(min_dist) && (a.movable || b.movable))
                {
                    const float dist = std::sqrt(dist_sq);
                    const auto dir = rel / dist;
                    const float delta = collision_response * (min_dist - dist);
                    auto [ka, kb] = MassCoefficients(a, b);
                    a.position += ka * delta * dir;
                    b.position -= kb * delta * dir;
                }
            }
        }
    }

    static std::tuple<float, float> MassCoefficients(const VerletObject& a, const VerletObject& b)
    {
        const float min_distance = a.radius + b.radius;
        if (a.movable)
        {
            if (b.movable)
            {
                return {a.radius / min_distance, b.radius / min_distance};
            }
            else
            {
                return {1.f, 0.f};
            }
        }
        else if (b.movable)
        {
            return {0.f, 1.f};
        }

        return {0.f, 0.f};
    }

    static void ApplyLinks(std::span<VerletObject> objects, std::span<const VerletLink> links)
    {
        for (const VerletLink& link : links)
        {
            VerletObject& a = objects[link.first];
            VerletObject& b = objects[link.second];
            Vec2f axis = a.position - b.position;
            const float distance = std::sqrt(axis.SquaredLength());
            axis /= distance;
            const float min_distance = a.radius + b.radius;
            const float delta = std::max(min_distance, link.target_distance) - distance;

            auto [ka, kb] = MassCoefficients(a, b);
            a.position += ka * delta * axis;
            b.position -= kb * delta * axis;
        }
    }

    [[nodiscard]] edt::Vec2f
    MakePreviousPosition(const edt::Vec2f& current_position, const edt::Vec2f& velocity, const float dt) const
    {
        return current_position - velocity / (dt * static_cast<float>(sub_steps));
    }
};
}  // namespace verlet
