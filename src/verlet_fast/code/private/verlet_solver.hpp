#pragma once

#include <ankerl/unordered_dense.h>

#include <cassert>

#include "EverydayTools/Concepts/Callable.hpp"
#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"
#include "EverydayTools/Template/TaggedIdentifier.hpp"
#include "tagged_id_hash.hpp"

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

struct WorldCellIdTag;
using WorldCellId = edt::TaggedIdentifier<
    WorldCellIdTag,
    edt::Vec2<int32_t>,
    edt::Vec2<int32_t>{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()}>;
static constexpr auto kInvalidCellId = WorldCellId{};

struct VerletObject
{
    Vec2f position{};
    Vec2f old_position{};
    Vec3<uint8_t> color;
    ObjectId next_in_cell = kInvalidObjectId;

    [[nodiscard]] static constexpr float GetRadius()
    {
        return 1.f;
    }

    bool movable = true;
};

struct VerletWorldCell
{
    ObjectId first = kInvalidObjectId;
};

struct VerletLink
{
    float target_distance{};
    ObjectId first{};
    ObjectId second{};
};

struct VerletWorld
{
    static constexpr Vec2f cell_size = Vec2f{} + VerletObject::GetRadius() * 6;

    void Reserve(const size_t num_objects)
    {
        objects.reserve(num_objects);
    }

    [[nodiscard]] constexpr auto GetObjectIds() const
    {
        return std::views::iota(0uz, objects.size()) | std::views::transform(ObjectId::FromValue);
    }

    VerletWorldCell* GetCell(const WorldCellId& cell_id)
    {
        assert(cell_id != kInvalidCellId);
        if (const auto it = cells.find(cell_id); it != cells.end())
        {
            return &it->second;
        }

        return nullptr;
    }

    VerletObject& GetObject(const ObjectId& id)
    {
        return objects[id.GetValue()];
    }

    template <edt::Callable<void, const ObjectId&, VerletObject&> Callback>
    void ForEachObjectInCell(const VerletWorldCell& cell, Callback&& callback)
    {
        auto object_id = cell.first;
        while (object_id != kInvalidObjectId)
        {
            auto& object = GetObject(object_id);
            const auto next = object.next_in_cell;
            callback(object_id, object);
            object_id = next;
        }
    }

    template <edt::Callable<void, const ObjectId&, VerletObject&> Callback>
    void ForEachObjectInCell(WorldCellId cell_id, Callback&& callback)
    {
        if (const VerletWorldCell* cell = GetCell(cell_id))
        {
            ForEachObjectInCell(*cell, std::forward<Callback>(callback));
        }
    }

    void RebuildGrid()
    {
        // Clear grid
        for (auto& [id, cell] : cells)
        {
            cell.first = kInvalidObjectId;
        }

        for (const auto object_id : GetObjectIds())
        {
            auto& object = GetObject(object_id);
            auto [cell_it, emplaced] = cells.try_emplace(CellIdByLocation(object.position));
            auto& cell = cell_it->second;
            if (emplaced)
            {
                // cell initialization. nothing to do here (for now)
            }

            object.next_in_cell = cell.first;
            cell.first = object_id;
        }
    }

    [[nodiscard]] static constexpr WorldCellId CellIdByLocation(const Vec2f& location)
    {
        return WorldCellId::FromValue((location / cell_size).Cast<int32_t>());
    }

    std::vector<VerletLink> links;
    std::vector<VerletObject> objects;
    ankerl::unordered_dense::map<WorldCellId, VerletWorldCell, TaggedIdentifierHash<WorldCellId>> cells;
};

struct VerletSolver
{
    edt::Vec2f gravity{0.0f, -0.75f};
    float constraint_radius = 1.f;
    size_t sub_steps = 8;

    void Update(VerletWorld& world, const float dt) const
    {
        const float sub_dt = dt / static_cast<float>(sub_steps);
        for ([[maybe_unused]] const size_t index : std::views::iota(0uz, sub_steps))
        {
            ApplyLinks(world.objects, world.links);
            world.RebuildGrid();
            SolveCollisions(world);
            UpdatePosition(world.objects, sub_dt);
            ApplyConstraint(world.objects);
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
                const float min_dist = constraint_radius - object.GetRadius();
                const float dist_sq = object.position.SquaredLength();
                if (dist_sq > edt::Math::Sqr(min_dist))
                {
                    const float dist = std::sqrt(dist_sq);
                    const auto direction = object.position / dist;
                    object.position = direction * (constraint_radius - object.GetRadius());
                }
            }
        }
    }

    void SolveCollisions(VerletWorld& world) const
    {
        auto solve_collision_between_object_and_cell =
            [&](const ObjectId& object_id, VerletObject& object, const WorldCellId& origin, const Vec2i& offset)
        {
            world.ForEachObjectInCell(
                WorldCellId::FromValue(origin.GetValue() + offset),
                [&](const ObjectId& another_object_id, VerletObject& another_object)
                {
                    if (object_id != another_object_id)
                    {
                        const float min_dist = object.GetRadius() + another_object.GetRadius();
                        const auto rel = object.position - another_object.position;
                        const float dist_sq = rel.SquaredLength();
                        if (dist_sq < edt::Math::Sqr(min_dist) && (object.movable || another_object.movable))
                        {
                            const float dist = std::sqrt(dist_sq);
                            const float delta = min_dist - dist;
                            auto [ka, kb] = MassCoefficients(object, another_object);
                            const auto coll_vec = delta * rel / dist;
                            object.position += ka * coll_vec;
                            another_object.position -= kb * coll_vec;
                        }
                    }
                });
        };

        for (auto& [cell_id, cell] : world.cells)
        {
            world.ForEachObjectInCell(
                cell,
                [&](const ObjectId& object_id, VerletObject& object)
                {
                    for (int dx = -1; dx != 2; ++dx)
                    {
                        for (int dy = -1; dy != 2; ++dy)
                        {
                            solve_collision_between_object_and_cell(object_id, object, cell_id, {dx, dy});
                        }
                    }
                });
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

    static void ApplyLinks(std::span<VerletObject> objects, std::span<const VerletLink> links)
    {
        for (const VerletLink& link : links)
        {
            VerletObject& a = objects[link.first.GetValue()];
            VerletObject& b = objects[link.second.GetValue()];
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

    [[nodiscard]] edt::Vec2f
    MakePreviousPosition(const edt::Vec2f& current_position, const edt::Vec2f& velocity, const float dt) const
    {
        return current_position - velocity / (dt * static_cast<float>(sub_steps));
    }
};
}  // namespace verlet
