#pragma once

#include <cstddef>
#include <cstdint>

namespace verlet
{
class VerletSolver;

class RandomObjectsParams
{
public:
    size_t count = 0;

    // Two spawns with the same seed and count produce the same objects, which is what makes
    // a measurement repeatable.
    uint32_t seed = 0;

    // World units per second. An object crossing more than its own radius between two
    // substeps can pass through another one before either notices, so the speed a spawn
    // asks for is capped at what a substep can resolve.
    float max_speed = 10.f;

    bool movable = true;
};

// Scatters objects over the solver's simulation area, each moving in a random direction.
void SpawnRandomObjects(VerletSolver& solver, const RandomObjectsParams& params);

}  // namespace verlet
