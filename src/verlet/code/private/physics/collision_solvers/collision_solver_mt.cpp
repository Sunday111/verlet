#include "collision_solver_mt.hpp"

#include <algorithm>
#include <functional>
#include <ranges>

#include "physics/verlet_solver.hpp"

namespace verlet
{
CollisionSolverMT::CollisionSolverMT(VerletSolver& solver, size_t threads_count)
    : sync_point_(static_cast<int32_t>(threads_count + 1))
{
    for (const size_t thread_index : std::views::iota(0uz, threads_count))
    {
        threads_.push_back(
            std::jthread(std::bind_front(&CollisionSolverMT::ThreadEntry, this), std::ref(solver), thread_index));
    }
}

CollisionSolverMT::~CollisionSolverMT()
{
    std::ranges::for_each(threads_, &std::jthread::request_stop);
    SolveCollisions();
    std::ranges::for_each(threads_, &std::jthread::join);
}

void CollisionSolverMT::SolveCollisions()
{
    sync_point_.arrive_and_wait();
    sync_point_.arrive_and_wait();
}

void CollisionSolverMT::ThreadEntry(const std::stop_token& stop_token, VerletSolver& solver, const size_t thread_index)
{
    while (!stop_token.stop_requested())
    {
        sync_point_.arrive_and_wait();
        solver.SolveCollisions(thread_index, threads_.size());
        sync_point_.arrive_and_wait();
    }
}
}  // namespace verlet
