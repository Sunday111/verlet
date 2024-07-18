#pragma once

#include <ankerl/unordered_dense.h>

#include <EverydayTools/Math/FloatRange.hpp>
#include <EverydayTools/Time/MeasureTime.hpp>
#include <barrier>
#include <cassert>
#include <thread>

#include "EverydayTools/Math/Math.hpp"
#include "EverydayTools/Math/Matrix.hpp"
#include "EverydayTools/Template/Overload.hpp"
#include "ankerl/unordered_dense.h"
#include "object_pool.hpp"
#include "tagged_id_hash.hpp"

namespace verlet
{

struct VerletSolver
{
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

    static constexpr size_t kNumThreads = 16;
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
        const auto& cell = cells[cell_index];
        return std::views::iota(uint8_t{0}, count) | std::views::transform(
                                                         [&](const uint8_t i) -> const ObjectId&
                                                         {
                                                             return cell.objects[i];
                                                         });
    }

    void WorkerThread(const std::stop_token& stop_token, const size_t thread_index);

    [[nodiscard]] size_t LocationToCellIndex(const Vec2f& location) const
    {
        auto [x, y] = ((sim_area_.Clamp(location) - sim_area_.Min()).Cast<size_t>() / cell_size).Tuple();
        return x + y * grid_size_.x();
    }

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

    void ApplyLinks();
    void RebuildGrid();
    UpdateStats Update();
    void UpdatePosition();
    void DeleteObject(ObjectId id);

    void SolveCollisions()
    {
        sync_point_.arrive_and_wait();  // ensure all threads at the beginning of the loop
        sync_point_.arrive_and_wait();  // ensure all threads finished collision detection
    }

    static std::tuple<float, float> MassCoefficients(const VerletObject& a, const VerletObject& b);

    std::vector<std::jthread> threads_;
    std::barrier<> sync_point_{kNumThreads + 1};

    edt::FloatRange2Df sim_area_ = {{-100, 100}, {-100, 100}};
    Vec2<size_t> grid_size_;

    ankerl::unordered_dense::map<ObjectId, std::vector<VerletLink>, TaggedIdentifierHash<ObjectId>> linked_to;
    ankerl::unordered_dense::map<ObjectId, std::vector<ObjectId>, TaggedIdentifierHash<ObjectId>> linked_by;

    ObjectPool objects;
    std::vector<VerletWorldCell> cells;
    std::vector<uint8_t> cell_obj_counts_;
};
}  // namespace verlet
