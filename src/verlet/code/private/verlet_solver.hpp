#pragma once

#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"
#include "verlet_objects.hpp"

namespace verlet
{
struct VerletSolver
{
    edt::Vec2f gravity{0.0f, -0.75f};
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
            const auto velocity = objects.position[index] - objects.old_position[index];

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
                const auto direction = objects.position[index] / dist;
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
                const auto rel = a_pos - b_pos;
                const float dist_sq = rel.SquaredLength();
                if (dist_sq < edt::Math::Sqr(min_dist))
                {
                    const float dist = std::sqrt(dist_sq);
                    const auto dir = rel / dist;
                    const float delta = 0.5f * collision_response * (min_dist - dist);

                    const float ka = a_r / min_dist;
                    a_pos += ka * delta * dir;

                    const float kb = b_r / min_dist;
                    b_pos -= kb * delta * dir;
                }
            }
        }
    }

    [[nodiscard]] edt::Vec2f
    MakePreviousPosition(const edt::Vec2f& current_position, const edt::Vec2f& velocity, const float dt) const
    {
        return current_position - velocity / (dt * static_cast<float>(sub_steps));
    }
};
}  // namespace verlet
