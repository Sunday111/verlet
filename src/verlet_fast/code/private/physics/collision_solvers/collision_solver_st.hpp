#pragma once

#include "collision_solver.hpp"

namespace verlet
{

class VerletSolver;

class CollisionSolverST : public CollisionSolver
{
public:
    explicit CollisionSolverST(VerletSolver& solver) : solver_(&solver) {}
    [[nodiscard]] size_t GetThreadsCount() const override
    {
        return 0;
    }
    void SolveCollisions() override;

    VerletSolver* solver_{};
};
}  // namespace verlet
