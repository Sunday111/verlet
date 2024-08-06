#pragma once

#include <cstddef>

namespace verlet
{
class CollisionSolver
{
public:
    virtual ~CollisionSolver() = default;
    [[nodiscard]] virtual size_t GetThreadsCount() const = 0;
    virtual void SolveCollisions() = 0;
};
}  // namespace verlet
