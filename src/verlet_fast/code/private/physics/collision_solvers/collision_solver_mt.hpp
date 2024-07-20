#pragma once

#include <barrier>
#include <thread>
#include <vector>

#include "collision_solver.hpp"

namespace verlet
{
class VerletSolver;
class CollisionSolverMT : public CollisionSolver
{
public:
    CollisionSolverMT(VerletSolver& solver, size_t threads_count);
    ~CollisionSolverMT() override;

    void ThreadEntry(const std::stop_token& stop_token, VerletSolver& solver, const size_t thread_index);
    [[nodiscard]] size_t GetThreadsCount() const override
    {
        return threads_.size();
    }
    void SolveCollisions() override;

    std::barrier<> sync_point_;
    std::vector<std::jthread> threads_;
};
}  // namespace verlet
