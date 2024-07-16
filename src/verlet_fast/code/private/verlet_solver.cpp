#include "verlet_solver.hpp"

#include "EverydayTools/Math/Math.hpp"
#include "ranges.hpp"

namespace verlet
{
VerletSolver::VerletSolver()
{
    for (const size_t thread_index : std::views::iota(0uz, kNumThreads))
    {
        threads_.push_back(std::jthread(std::bind_front(&VerletSolver::WorkerThread, this), thread_index));
    }
}

VerletSolver::~VerletSolver()
{
    std::ranges::for_each(threads_, &std::jthread::request_stop);
    SolveCollisions();
}

void VerletSolver::WorkerThread(const std::stop_token& stop_token, const size_t thread_index)
{
    while (!stop_token.stop_requested())
    {
        // waint for request
        sync_point_.arrive_and_wait();

        constexpr float eps = 0.0001f;
        auto solve_collision_between_object_and_cell =
            [&](const ObjectId& object_id, VerletObject& object, const size_t origin_cell_index)
        {
            for (const ObjectId& another_object_id : ForEachObjectInCell(origin_cell_index))
            {
                if (object_id != another_object_id)
                {
                    auto& another_object = GetObject(another_object_id);
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
            }
        };

        const size_t columns_pre_thread = (grid_size_.x() / kNumThreads);
        size_t begin_x = 1 + columns_pre_thread * thread_index;
        size_t end_x = begin_x + columns_pre_thread;
        if (thread_index == kNumThreads - 1)
        {
            end_x = grid_size_.x();
        }

        const size_t grid_width = grid_size_.x();
        for (const size_t cell_x : std::views::iota(begin_x, end_x))
        {
            for (const size_t cell_y : std::views::iota(1uz, grid_size_.y() - 1))
            {
                const size_t cell_index = cell_y * grid_width + cell_x;
                for (const ObjectId& object_id : ForEachObjectInCell(cell_index))
                {
                    auto& object = GetObject(object_id);
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

        // notify completion
        sync_point_.arrive_and_wait();
    }
}

void VerletSolver::RebuildGrid()
{
    // Clear grid
    grid_size_ = Vec2<size_t>{2, 2} + sim_area_.Extent().Cast<size_t>() / cell_size;
    const size_t cells_count = grid_size_.x() * grid_size_.y();
    cell_obj_counts_.resize(cells_count);
    cells.resize(cells_count);

    std::ranges::fill(cell_obj_counts_, uint8_t{0});

    for (auto [object_index, object] : Enumerate(objects))
    {
        const auto cell_index = LocationToCellIndex(object.position);
        auto& cell = cells[cell_index];
        auto& cell_sz = cell_obj_counts_[cell_index];
        cell.objects[cell_sz % VerletWorldCell::kCapacity] = ObjectId::FromValue(object_index);
        cell_sz = std::min<uint8_t>(cell_sz + 1, VerletWorldCell::kCapacity);
    }
}

VerletSolver::UpdateStats VerletSolver::Update()
{
    UpdateStats stats{};
    stats.total = edt::MeasureTime(
        [&]
        {
            for ([[maybe_unused]] const size_t index : std::views::iota(0uz, kNumSubSteps))
            {
                stats.apply_links += edt::MeasureTime(std::bind_front(&VerletSolver::ApplyLinks, this));
                stats.rebuild_grid += edt::MeasureTime(std::bind_front(&VerletSolver::RebuildGrid, this));
                stats.solve_collisions += edt::MeasureTime(std::bind_front(&VerletSolver::SolveCollisions, this));
                stats.update_positions += edt::MeasureTime(std::bind_front(&VerletSolver::UpdatePosition, this));
            }
        });

    return stats;
}

void VerletSolver::UpdatePosition()
{
    constexpr float margin = 2.0f;
    const auto constraint_with_margin = sim_area_.Enlarged(-margin);
    constexpr float dt_2 = edt::Math::Sqr(kTimeSubStepDurationSeconds);

    for (auto& object : objects)
    {
        [[likely]] if (object.movable)
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

}  // namespace verlet
