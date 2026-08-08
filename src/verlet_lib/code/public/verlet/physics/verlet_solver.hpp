#pragma once

#include <ankerl/unordered_dense.h>

#include <cassert>
#include <edt/math/float_range.hpp>
#include <edt/time/measure_time.hpp>

#include "edt/math/math.hpp"
#include "edt/math/matrix.hpp"
#include "edt/template/overload.hpp"
#include "klvk/template/tagged_id_hash.hpp"
#include "verlet/object_pool.hpp"

namespace edt
{
class BatchThreadPool;
}

namespace verlet
{

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

    // A cell keeps only the first of its objects; each object names the next one, so the
    // chain is walked rather than indexed.
    class CellObjects : public std::ranges::view_interface<CellObjects>
    {
    public:
        class Iterator
        {
        public:
            using value_type = ObjectId;
            using difference_type = std::ptrdiff_t;

            Iterator() = default;
            Iterator(const ObjectPool* pool, uint32_t index) : pool_{pool}, index_{index} {}

            [[nodiscard]] ObjectId operator*() const { return ObjectId::FromValue(index_); }

            Iterator& operator++()
            {
                index_ = pool_->Get(ObjectId::FromValue(index_)).next_object_in_cell;
                return *this;
            }

            void operator++(int) { ++*this; }

            [[nodiscard]] bool operator==(std::default_sentinel_t) const { return index_ == kInvalidObjectIndex; }

        private:
            const ObjectPool* pool_ = nullptr;
            uint32_t index_ = kInvalidObjectIndex;
        };

        CellObjects() = default;
        CellObjects(const ObjectPool& pool, uint32_t first) : pool_{&pool}, first_{first} {}

        [[nodiscard]] Iterator begin() const { return Iterator{pool_, first_}; }
        [[nodiscard]] std::default_sentinel_t end() const { return {}; }  // NOLINT

    private:
        const ObjectPool* pool_ = nullptr;
        uint32_t first_ = kInvalidObjectIndex;
    };

    static constexpr float kVelocityDampling = 40.f;  // arbitrary, approximating air friction
    static constexpr edt::Vec2f gravity{0.0f, -20.f};
    static constexpr Vec2<size_t> cell_size{1, 1};
    static constexpr size_t kCollisionPassStride = 3;
    static constexpr float kTimeStepDurationSeconds = 1.f / 60.f;
    static constexpr size_t kNumSubSteps = 8;
    static constexpr float kTimeSubStepDurationSeconds = kTimeStepDurationSeconds / static_cast<float>(kNumSubSteps);

    VerletSolver();
    VerletSolver(const VerletSolver&) = delete;
    VerletSolver(VerletSolver&&) = delete;
    ~VerletSolver();

    [[nodiscard]] CellObjects ForEachObjectInCell(const size_t cell_index) const
    {
        return CellObjects{objects, cell_heads_[cell_index]};
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

            return std::views::filter(
                edt::Overload{
                    is_movable,
                    [](const std::tuple<ObjectId, const VerletObject&> id_and_obj)
                    { return std::get<1>(id_and_obj).movable; }});
        }

        [[nodiscard]] static auto InArea(Vec2f position, float radius)
        {
            auto is_close_enough = [position, rsq = edt::Math::Sqr(radius)](const VerletObject& object)
            {
                return (position - object.position).SquaredLength() < rsq;
            };

            return std::views::filter(
                edt::Overload{
                    is_close_enough,
                    [=](const std::tuple<ObjectId, const VerletObject&> id_and_obj)
                    { return is_close_enough(std::get<1>(id_and_obj)); }});
        }
    };

    UpdateStats Update();
    void ApplyLinks();
    void RebuildGrid();
    void SolveCollisions(size_t pass_offset, size_t thread_index, size_t threads_count);
    void UpdatePositions(size_t thread_index, size_t threads_count);

    void DeleteObject(ObjectId id);
    void DeleteAll();
    void StabilizeChain(ObjectId first);
    void CreateLink(ObjectId from, ObjectId to, float target_distance);

    [[nodiscard]] size_t GetThreadsCount() const;
    void SetThreadsCount(size_t count);

    [[nodiscard]] const edt::FloatRange2Df& GetSimArea() const { return sim_area_; }
    void SetSimArea(const edt::FloatRange2Df& sim_area);

    ObjectPool objects;

private:
    static std::tuple<float, float> MassCoefficients(const VerletObject& a, const VerletObject& b);
    void UpdateGridSize();

private:
    edt::FloatRange2Df sim_area_ = {.x = {.begin = -100, .end = 100}, .y = {.begin = -100, .end = 100}};
    bool sim_area_changed_ = true;

    bool update_in_progress_ = false;
    Vec2<size_t> grid_size_;

    std::vector<uint32_t> cell_heads_;
    std::unique_ptr<edt::BatchThreadPool> batch_thread_pool_;

    // links
    ankerl::unordered_dense::map<ObjectId, std::vector<VerletLink>, klvk::TaggedIdentifierHash<ObjectId>> linked_to;
    ankerl::unordered_dense::map<ObjectId, std::vector<ObjectId>, klvk::TaggedIdentifierHash<ObjectId>> linked_by;
};

}  // namespace verlet
