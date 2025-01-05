#pragma once

#include <ankerl/unordered_dense.h>

#include <EverydayTools/Math/FloatRange.hpp>
#include <EverydayTools/Time/MeasureTime.hpp>
#include <cassert>

#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"
#include "EverydayTools/Template/Overload.hpp"
#include "klgl/template/tagged_id_hash.hpp"
#include "verlet/object_pool.hpp"

namespace verlet
{

class BatchThreadPool;

class VerletSolver
{
public:
    struct UpdateStats
    {
        std::chrono::nanoseconds apply_links;
        std::chrono::nanoseconds rebuild_grid;
        std::chrono::nanoseconds solve_collisions;
        std::chrono::nanoseconds update_positions;
        std::chrono::nanoseconds total;
    };

    struct VerletLink
    {
        float target_distance{};
        ObjectId other{};
    };

    struct VerletWorldCell
    {
        static constexpr uint8_t kCapacity = 4;
        std::array<ObjectId, kCapacity> objects;
    };

    static constexpr float kVelocityDampling = 40.f;  // arbitrary, approximating air friction
    static constexpr edt::Vec2f gravity{0.0f, -20.f};
    static constexpr Vec2<size_t> cell_size{1, 1};
    static constexpr float kTimeStepDurationSeconds = 1.f / 60.f;
    static constexpr size_t kNumSubSteps = 8;
    static constexpr float kTimeSubStepDurationSeconds = kTimeStepDurationSeconds / static_cast<float>(kNumSubSteps);

    VerletSolver();
    VerletSolver(const VerletSolver&) = delete;
    VerletSolver(VerletSolver&&) = delete;
    ~VerletSolver();

    [[nodiscard]] auto ForEachObjectInCell(const size_t cell_index) const
    {
        const uint8_t count = cell_obj_counts_[cell_index];
        const auto& cell = cells_[cell_index];
        return std::views::iota(uint8_t{0}, count) |
               std::views::transform([&](const uint8_t i) -> const ObjectId& { return cell.objects[i]; });
    }

    [[nodiscard]] Vec2<size_t> LocationToCell(const Vec2f& location) const
    {
        return ((sim_area_.Clamp(location) - sim_area_.Min()).Cast<size_t>() / cell_size);
    }

    [[nodiscard]] size_t LocationToCellIndex(const Vec2f& location) const
    {
        return CellToCellIndex(LocationToCell(location));
    }

    [[nodiscard]] size_t CellToCellIndex(const Vec2<size_t>& cell) const
    {
        return cell.x() + cell.y() * grid_size_.x();
    }

    struct ObjectTransforms
    {
        [[nodiscard]] static auto IdToObject(VerletSolver& solver)
        {
            return std::views::transform([&](const ObjectId& id) -> VerletObject& { return solver.objects.Get(id); });
        }
    };

    struct ObjectFilters
    {
        [[nodiscard]] static auto IsMovable()
        {
            constexpr auto is_movable = [](const VerletObject& object)
            {
                return object.IsMovable();
            };

            return std::views::filter(edt::Overload{
                is_movable,
                [](const std::tuple<ObjectId, const VerletObject&> id_and_obj)
                {
                    return std::get<1>(id_and_obj).movable;
                }});
        }

        [[nodiscard]] static auto InArea(Vec2f position, float radius)
        {
            auto is_close_enough = [position, rsq = edt::Math::Sqr(radius)](const VerletObject& object)
            {
                return (position - object.position).SquaredLength() < rsq;
            };

            return std::views::filter(edt::Overload{
                is_close_enough,
                [=](const std::tuple<ObjectId, const VerletObject&> id_and_obj)
                {
                    return is_close_enough(std::get<1>(id_and_obj));
                }});
        }
    };

    UpdateStats Update();
    void ApplyLinks();
    void RebuildGrid();
    void SolveCollisions(const size_t thread_index, const size_t threads_count);
    void UpdatePositions(const size_t thread_index, const size_t threads_count);

    void DeleteObject(ObjectId id);
    void DeleteAll();
    void StabilizeChain(ObjectId first);
    void CreateLink(ObjectId from, ObjectId to, float target_distance);

    size_t GetThreadsCount() const;
    void SetThreadsCount(size_t count);

    [[nodiscard]] const edt::FloatRange2Df& GetSimArea() const { return sim_area_; }
    void SetSimArea(const edt::FloatRange2Df& sim_area);

    ObjectPool objects;

private:
    static std::tuple<float, float> MassCoefficients(const VerletObject& a, const VerletObject& b);
    void UpdateGridSize();

private:
    edt::FloatRange2Df sim_area_ = {{-100, 100}, {-100, 100}};
    bool sim_area_changed_ = true;

    bool update_in_progress_ = false;
    Vec2<size_t> grid_size_;

    std::vector<VerletWorldCell> cells_;
    std::vector<uint8_t> cell_obj_counts_;
    std::unique_ptr<BatchThreadPool> batch_thread_pool_;

    // links
    ankerl::unordered_dense::map<ObjectId, std::vector<VerletLink>, klgl::TaggedIdentifierHash<ObjectId>> linked_to;
    ankerl::unordered_dense::map<ObjectId, std::vector<ObjectId>, klgl::TaggedIdentifierHash<ObjectId>> linked_by;
};

}  // namespace verlet
