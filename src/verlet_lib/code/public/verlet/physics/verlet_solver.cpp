#include "verlet_solver.hpp"

#include "EverydayTools/Math/Math.hpp"
#include "fmt/ranges.h"  // IWYU pragma: keep
#include "klgl/error_handling.hpp"
#include "klgl/template/on_scope_leave.hpp"
#include "verlet/threading/batch_thread_pool.hpp"

namespace verlet
{
VerletSolver::VerletSolver()
{
    SetThreadsCount(std::thread::hardware_concurrency());
}

void VerletSolver::SolveCollisions(const size_t thread_index, size_t threads_count)
{
    constexpr float eps = 0.0001f;
    auto solve_collision_between_object_and_cell =
        [&](const ObjectId& object_id, VerletObject& object, const size_t origin_cell_index)
    {
        for (const ObjectId& another_object_id : ForEachObjectInCell(origin_cell_index))
        {
            if (object_id != another_object_id)
            {
                auto& another_object = objects.Get(another_object_id);
                const Vec2f axis = object.position - another_object.position;
                const float dist_sq = axis.SquaredLength();
                if (dist_sq < 1.0f && dist_sq > eps)
                {
                    const float dist = std::sqrt(dist_sq);
                    const float delta = 0.5f - dist / 2;
                    const Vec2f col_vec = axis * (delta / dist);
                    const auto [ac, bc] = MassCoefficients(object, another_object);
                    object.position += ac * col_vec;
                    another_object.position -= bc * col_vec;
                }
            }
        }
    };

    const size_t columns_pre_thread = (grid_size_.x() / threads_count);
    size_t begin_x = 1 + columns_pre_thread * thread_index;
    size_t end_x = begin_x + columns_pre_thread;
    if (thread_index == threads_count - 1)
    {
        end_x = grid_size_.x();
    }

    const size_t grid_width = grid_size_.x();
    for (const size_t cell_x : std::views::iota(begin_x, end_x))
    {
        for (const size_t cell_y : std::views::iota(size_t{1}, grid_size_.y() - 1))
        {
            const size_t cell_index = cell_y * grid_width + cell_x;
            for (const ObjectId& object_id : ForEachObjectInCell(cell_index))
            {
                auto& object = objects.Get(object_id);
                solve_collision_between_object_and_cell(object_id, object, cell_index);
                solve_collision_between_object_and_cell(object_id, object, cell_index + 1);
                solve_collision_between_object_and_cell(object_id, object, cell_index - 1);
                solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width);
                solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width + 1);
                solve_collision_between_object_and_cell(object_id, object, cell_index + grid_width - 1);
                solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width);
                solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width + 1);
                solve_collision_between_object_and_cell(object_id, object, cell_index - grid_width - 1);
            }
        }
    }
}

void VerletSolver::RebuildGrid()
{
    if (sim_area_changed_)
    {
        UpdateGridSize();
        sim_area_changed_ = false;
    }

    std::ranges::fill(cell_obj_counts_, uint8_t{0});

    for (auto [id, object] : objects.IdentifiersAndObjects())
    {
        const auto cell_index = LocationToCellIndex(object.position);
        auto& cell = cells_[cell_index];
        auto& cell_sz = cell_obj_counts_[cell_index];
        cell.objects[cell_sz % VerletWorldCell::kCapacity] = id;
        cell_sz = std::min<uint8_t>(cell_sz + 1, VerletWorldCell::kCapacity);
    }
}

VerletSolver::UpdateStats VerletSolver::Update()
{
    update_in_progress_ = true;
    const auto scope_leave_ = klgl::OnScopeLeave([this] { update_in_progress_ = false; });
    UpdateStats stats{};
    stats.total = edt::MeasureTime(
        [&]
        {
            for ([[maybe_unused]] const size_t index : std::views::iota(size_t{0}, kNumSubSteps))
            {
                stats.rebuild_grid += edt::MeasureTime(std::bind_front(&VerletSolver::RebuildGrid, this));
                stats.apply_links += edt::MeasureTime(std::bind_front(&VerletSolver::ApplyLinks, this));
                stats.solve_collisions += edt::MeasureTime(
                    [&] { batch_thread_pool_->RunBatch(std::bind_front(&VerletSolver::SolveCollisions, this)); });
                stats.update_positions += edt::MeasureTime(
                    [&] { batch_thread_pool_->RunBatch(std::bind_front(&VerletSolver::UpdatePositions, this)); });
            }
        });

    return stats;
}

void VerletSolver::UpdatePositions(const size_t thread_index, const size_t threads_count)
{
    constexpr float margin = 2.0f;
    const auto constraint_with_margin = sim_area_.Enlarged(-margin);
    constexpr float dt_2 = edt::Math::Sqr(kTimeSubStepDurationSeconds);

    const size_t columns_pre_thread = (grid_size_.x() / threads_count);
    size_t begin_x = 1 + columns_pre_thread * thread_index;
    size_t end_x = begin_x + columns_pre_thread;
    if (thread_index == threads_count - 1)
    {
        end_x = grid_size_.x();
    }

    const size_t grid_width = grid_size_.x();
    for (const size_t cell_x : std::views::iota(begin_x, end_x))
    {
        for (const size_t cell_y : std::views::iota(size_t{1}, grid_size_.y() - 1))
        {
            const size_t cell_index = cell_y * grid_width + cell_x;
            for (auto& object :
                 ForEachObjectInCell(cell_index) | ObjectTransforms::IdToObject(*this) | ObjectFilters::IsMovable())
            {
                const auto last_update_move = object.position - object.old_position;

                // Save current position
                object.old_position = object.position;

                // Perform Verlet integration
                object.position += last_update_move + (gravity - last_update_move * kVelocityDampling) * dt_2;

                // Constraint
                object.position = constraint_with_margin.Clamp(object.position);
            }
        }
    }
}

void VerletSolver::DeleteAll()
{
    linked_to.clear();
    linked_by.clear();
    objects.Clear();
}

void VerletSolver::DeleteObject(ObjectId to_delete)
{
    if (auto linked_by_it = linked_by.find(to_delete); linked_by_it != linked_by.end())
    {
        for (auto& other : linked_by_it->second)
        {
            auto& v = linked_to[other];
            auto [b, e] = std::ranges::remove(v, to_delete, &VerletSolver::VerletLink::other);
            v.erase(b, e);
        }

        linked_by.erase(linked_by_it);
    }

    if (auto linked_to_it = linked_to.find(to_delete); linked_to_it != linked_to.end())
    {
        for (auto& link : linked_to_it->second)
        {
            auto& v = linked_by[link.other];
            auto [b, e] = std::ranges::remove(v, to_delete);
            v.erase(b, e);
        }
    }

    linked_to.erase(to_delete);
    objects.Free(to_delete);
}

void VerletSolver::StabilizeChain(ObjectId first)
{
    std::vector queue{first};
    ankerl::unordered_dense::set<ObjectId, TaggedIdentifierHash<ObjectId>> visited;

    while (!queue.empty())
    {
        ObjectId id = queue.back();
        queue.pop_back();

        if (!visited.emplace(id).second)
        {
            continue;
        }

        if (auto it = linked_to.find(id); it != linked_to.end())
        {
            std::ranges::copy(
                it->second | std::views::transform(std::mem_fn(&VerletSolver::VerletLink::other)),
                std::back_inserter(queue));
        }

        if (auto it = linked_by.find(id); it != linked_by.end())
        {
            std::ranges::copy(it->second, std::back_inserter(queue));
        }

        auto& object = objects.Get(id);
        object.old_position = object.position;
    }
}

std::tuple<float, float> VerletSolver::MassCoefficients(const VerletObject& a, const VerletObject& b)
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

void VerletSolver::ApplyLinks()
{
    for (const auto& [object_id, links] : linked_to)
    {
        VerletObject& a = objects.Get(object_id);

        for (const auto& link : links)
        {
            VerletObject& b = objects.Get(link.other);

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
}

size_t VerletSolver::GetThreadsCount() const
{
    return batch_thread_pool_->GetThreadsCount();
}

void VerletSolver::SetThreadsCount(size_t count)
{
    if (!batch_thread_pool_ || count != GetThreadsCount())
    {
        batch_thread_pool_ = std::make_unique<BatchThreadPool>(count);
    }
}

void VerletSolver::SetSimArea(const edt::FloatRange2Df& sim_area)
{
    klgl::ErrorHandling::Ensure(!update_in_progress_, "Attempt to change simulation area while update is in progress");
    if (sim_area.Min() != sim_area_.Min() || sim_area.Max() != sim_area_.Max())
    {
        sim_area_ = sim_area;
        sim_area_changed_ = true;
    }
}

void VerletSolver::UpdateGridSize()
{
    grid_size_ = Vec2<size_t>{2, 2} + sim_area_.Extent().Cast<size_t>() / cell_size;
    const size_t cells_count = grid_size_.x() * grid_size_.y();
    cell_obj_counts_.resize(cells_count);
    cells_.resize(cells_count);
}

VerletSolver::~VerletSolver()
{
    batch_thread_pool_ = nullptr;
}

void VerletSolver::CreateLink(ObjectId from, ObjectId to, float target_distance)
{
    linked_to[from].push_back({
        .target_distance = target_distance,
        .other = to,
    });

    linked_by[to].push_back(from);
}

}  // namespace verlet
