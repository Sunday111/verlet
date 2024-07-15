#pragma once

#include <ankerl/unordered_dense.h>

#include <EverydayTools/Math/FloatRange.hpp>
#include <EverydayTools/Time/MeasureTime.hpp>
#include <cassert>

#include "EverydayTools/Concepts/Callable.hpp"
#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"
#include "EverydayTools/Template/TaggedIdentifier.hpp"

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

template <typename T, auto extent = std::dynamic_extent>
[[nodiscard]] constexpr inline auto IndicesView(std::span<T, extent> span)
{
    return std::views::iota(0uz, span.size());
}

struct ObjectIdTag;
using ObjectId = edt::TaggedIdentifier<ObjectIdTag, size_t>;
static constexpr auto kInvalidObjectId = ObjectId{};

struct VerletObject
{
    Vec2f position{};
    Vec2f old_position{};
    Vec3<uint8_t> color;
    ObjectId next_in_cell = kInvalidObjectId;

    [[nodiscard]] static constexpr float GetRadius()
    {
        return 0.5f;
    }

    bool movable = true;
};

struct VerletWorldCell
{
    static constexpr uint8_t kCapacity = 4;
    std::array<ObjectId, kCapacity> objects;
};

struct VerletLink
{
    float target_distance{};
    ObjectId first{};
    ObjectId second{};
};

struct VerletWorld
{
    static constexpr Vec2<size_t> cell_size{1, 1};
    static constexpr float kTimeStepDurationSeconds = 1.f / 60.f;
    static constexpr size_t kNumSubSteps = 8;
    static constexpr float kTimeSubStepDurationSeconds = kTimeStepDurationSeconds / static_cast<float>(kNumSubSteps);

    void Reserve(const size_t num_objects)
    {
        objects.reserve(num_objects);
    }

    [[nodiscard]] constexpr auto GetObjectIds() const
    {
        return std::views::iota(0uz, objects.size()) | std::views::transform(ObjectId::FromValue);
    }

    VerletObject& GetObject(const ObjectId& id)
    {
        return objects[id.GetValue()];
    }

    template <edt::Callable<void, const ObjectId&> Callback>
    void ForEachObjectInCell(const size_t cell_index, Callback&& callback)
    {
        const uint8_t count = cell_obj_counts_[cell_index];
        const auto& cell = cells[cell_index];
        for (uint8_t i = 0; i != count; ++i)
        {
            callback(cell.objects[i]);
        }
    }

    [[nodiscard]] size_t LocationToCellIndex(const Vec2f& location) const
    {
        auto [x, y] = ((location - sim_area_.Min()).Cast<size_t>() / cell_size).Tuple();
        return x + y * grid_size_.x();
    }

    void RebuildGrid()
    {
        // Clear grid
        grid_size_ = Vec2<size_t>{2, 2} + sim_area_.Extent().Cast<size_t>() / cell_size;
        const size_t cells_count = grid_size_.x() * grid_size_.y();
        cell_obj_counts_.resize(cells_count);
        cells.resize(cells_count);

        std::ranges::fill(cell_obj_counts_, uint8_t{0});

        for (const auto object_id : GetObjectIds())
        {
            auto& object = GetObject(object_id);
            const auto cell_index = LocationToCellIndex(object.position);
            auto& cell = cells[cell_index];
            auto& cell_sz = cell_obj_counts_[cell_index];
            cell.objects[cell_sz % VerletWorldCell::kCapacity] = object_id;
            cell_sz = std::min<uint8_t>(cell_sz + 1, VerletWorldCell::kCapacity);
        }
    }

    edt::FloatRange2Df sim_area_ = {{-100, 100}, {-100, 100}};
    Vec2<size_t> grid_size_;
    std::vector<VerletLink> links;
    std::vector<VerletObject> objects;
    std::vector<VerletWorldCell> cells;
    std::vector<uint8_t> cell_obj_counts_;
};

struct VerletSolver
{
    static constexpr edt::Vec2f gravity{0.0f, -200.f};

    struct UpdateStats
    {
        std::chrono::nanoseconds apply_links;
        std::chrono::nanoseconds rebuild_grid;
        std::chrono::nanoseconds solve_collisions;
        std::chrono::nanoseconds update_positions;
        std::chrono::nanoseconds total;
    };

    UpdateStats Update(VerletWorld& world) const
    {
        UpdateStats stats{};
        stats.total = edt::MeasureTime(
            [&]
            {
                for ([[maybe_unused]] const size_t index : std::views::iota(0uz, VerletWorld::kNumSubSteps))
                {
                    stats.apply_links += edt::MeasureTime(std::bind_front(&VerletSolver::ApplyLinks), world);
                    stats.rebuild_grid += edt::MeasureTime(std::bind_front(&VerletWorld::RebuildGrid, &world));
                    stats.solve_collisions +=
                        edt::MeasureTime(std::bind_front(&VerletSolver::SolveCollisions, this), world);
                    stats.update_positions +=
                        edt::MeasureTime(std::bind_front(&VerletSolver::UpdatePosition, this), world);
                }
            });

        return stats;
    }

    void UpdatePosition(VerletWorld& world) const
    {
        constexpr float margin = 2.0f;
        const auto constraint_with_margin = world.sim_area_.Enlarged(-margin);
        constexpr auto gravity_influence = gravity * edt::Math::Sqr(VerletWorld::kTimeSubStepDurationSeconds);

        for (auto& object : world.objects)
        {
            [[likely]] if (object.movable)
            {
                const auto velocity = object.position - object.old_position;

                // Save current position
                object.old_position = object.position;

                // Perform Verlet integration
                object.position += velocity + gravity_influence;

                ApplyConstraint(object, constraint_with_margin);
            }
        }
    }

    void ApplyConstraint(VerletObject& object, const edt::FloatRange2D<float>& constraint) const
    {
        if (object.movable)
        {
            object.position = constraint.Clamp(object.position);
        }
    }

    void SolveCollisions(VerletWorld& world) const
    {
        constexpr float eps = 0.0001f;
        auto solve_collision_between_object_and_cell =
            [&](const ObjectId& object_id, VerletObject& object, const size_t origin_cell_index)
        {
            world.ForEachObjectInCell(
                origin_cell_index,
                [&](const ObjectId& another_object_id)
                {
                    if (object_id != another_object_id)
                    {
                        auto& another_object = world.GetObject(another_object_id);
                        const Vec2f axis = object.position - another_object.position;
                        const float dist_sq = axis.SquaredLength();
                        if (dist_sq < 1.0f && dist_sq > eps)
                        {
                            const float dist = std::sqrt(dist_sq);
                            const float delta = 0.5f - dist / 2;
                            const Vec2f col_vec = (axis / dist) * delta;
                            object.position += col_vec;
                            another_object.position -= col_vec;
                        }
                    }
                });
        };

        const size_t grid_width = world.grid_size_.x();
        for (const size_t cell_y : std::views::iota(1uz, world.grid_size_.y() - 1))
        {
            const size_t offset = cell_y * grid_width;
            for (const size_t cell_x : std::views::iota(1uz, grid_width - 1))
            {
                const size_t cell_index = offset + cell_x;
                world.ForEachObjectInCell(
                    cell_index,
                    [&](const ObjectId& object_id)
                    {
                        auto& object = world.GetObject(object_id);
                        solve_collision_between_object_and_cell(object_id, object, cell_index);
                        solve_collision_between_object_and_cell(object_id, object, cell_index + 1);
                        solve_collision_between_object_and_cell(object_id, object, cell_index - 1);
                        solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width);
                        solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width + 1);
                        solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width - 1);
                        solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width);
                        solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width + 1);
                        solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width - 1);
                    });
            }
        }
    }

    static std::tuple<float, float> MassCoefficients(const VerletObject& a, const VerletObject& b)
    {
        const float min_distance = a.GetRadius() + b.GetRadius();
        if (a.movable)
        {
            if (b.movable)
            {
                return {a.GetRadius() / min_distance, b.GetRadius() / min_distance};
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

    static void ApplyLinks(VerletWorld& world)
    {
        for (const VerletLink& link : world.links)
        {
            VerletObject& a = world.objects[link.first.GetValue()];
            VerletObject& b = world.objects[link.second.GetValue()];
            Vec2f axis = a.position - b.position;
            const float distance = std::sqrt(axis.SquaredLength());
            axis /= distance;
            const float min_distance = a.GetRadius() + b.GetRadius();
            const float delta = std::max(min_distance, link.target_distance) - distance;

            auto [ka, kb] = MassCoefficients(a, b);
            a.position += ka * delta * axis;
            b.position -= kb * delta * axis;
        }
    }

    [[nodiscard]] edt::Vec2f MakePreviousPosition(const edt::Vec2f& current_position, const edt::Vec2f& velocity) const
    {
        return current_position - velocity / VerletWorld::kTimeSubStepDurationSeconds;
    }
};
}  // namespace verlet
