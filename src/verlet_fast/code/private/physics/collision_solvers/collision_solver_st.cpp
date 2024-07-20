#include "collision_solver_st.hpp"

#include "physics/verlet_solver.hpp"

namespace verlet
{
void CollisionSolverST::SolveCollisions()
{
    solver_->SolveCollisions(0, 1);
}
}  // namespace verlet
