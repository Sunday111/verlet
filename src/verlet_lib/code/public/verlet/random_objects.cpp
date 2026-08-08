#include "random_objects.hpp"

#include <algorithm>
#include <random>

#include "edt/math/math.hpp"
#include "verlet/physics/verlet_solver.hpp"

namespace verlet
{
namespace
{
// std::uniform_real_distribution is free to differ between standard libraries, and a spawn
// that depends on which one it was built against is not reproducible.
class Random
{
public:
    explicit Random(uint32_t seed) : engine_{seed} {}

    [[nodiscard]] float UnitInterval()
    {
        constexpr uint32_t kMantissaBits = 24;
        constexpr float kScale = 1.f / static_cast<float>(1U << kMantissaBits);
        return static_cast<float>(engine_() >> (32 - kMantissaBits)) * kScale;
    }

    [[nodiscard]] float Between(float min, float max) { return min + (max - min) * UnitInterval(); }

private:
    std::mt19937 engine_;
};
}  // namespace

void SpawnRandomObjects(VerletSolver& solver, const RandomObjectsParams& params)
{
    // What UpdatePositions clamps to, less the radius, so a spawned object starts inside the
    // area it will be held in rather than being pulled to the edge on its first step.
    constexpr float margin = 2.f + VerletObject::GetRadius();
    const auto area = solver.GetSimArea().Enlarged(-margin);

    constexpr float kMaxResolvableSpeed = VerletObject::GetRadius() / VerletSolver::kTimeSubStepDurationSeconds;
    const float max_speed = std::clamp(params.max_speed, 0.f, kMaxResolvableSpeed);

    Random random{params.seed};
    for ([[maybe_unused]] const size_t index : std::views::iota(size_t{0}, params.count))
    {
        const Vec2f position{random.Between(area.x.begin, area.x.end), random.Between(area.y.begin, area.y.end)};

        const float direction = random.Between(0.f, 2 * std::numbers::pi_v<float>);
        const float speed = random.Between(0.f, max_speed);
        const Vec2f velocity = speed * Vec2f{std::cos(direction), std::sin(direction)};

        auto [id, object] = solver.objects.Alloc();
        std::ignore = id;
        object.position = position;
        object.old_position = position - velocity * VerletSolver::kTimeSubStepDurationSeconds;
        object.movable = params.movable;

        const auto rgb = edt::Math::GetRainbowColors(random.UnitInterval());
        object.color = {rgb.x(), rgb.y(), rgb.z(), 255};
    }
}

}  // namespace verlet
