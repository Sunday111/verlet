#include "verlet/physics/verlet_solver.hpp"

#include <vector>

#include "gtest/gtest.h"

namespace
{
constexpr size_t kObjectsPerSide = 40;
constexpr float kSpacing = 0.8f;
constexpr size_t kSteps = 200;

static_assert(kSpacing < 2 * verlet::VerletObject::GetRadius());

std::vector<edt::Vec2f> Simulate(size_t threads_count, size_t steps)
{
    verlet::VerletSolver solver;
    solver.SetThreadsCount(threads_count);

    const auto origin = solver.GetSimArea().Min() + 10.f;
    for (size_t y = 0; y != kObjectsPerSide; ++y)
    {
        for (size_t x = 0; x != kObjectsPerSide; ++x)
        {
            auto [id, object] = solver.objects.Alloc();
            std::ignore = id;
            object.position = origin + edt::Vec2f{static_cast<float>(x), static_cast<float>(y)} * kSpacing;
            object.old_position = object.position;
            object.movable = true;
        }
    }

    for (size_t step = 0; step != steps; ++step) std::ignore = solver.Update();

    std::vector<edt::Vec2f> positions;
    positions.reserve(solver.objects.ObjectsCount());
    for (const auto& object : solver.objects.Objects()) positions.push_back(object.position);
    return positions;
}

void ExpectSamePositions(const std::vector<edt::Vec2f>& expected, const std::vector<edt::Vec2f>& actual)
{
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i != expected.size(); ++i)
    {
        EXPECT_EQ(expected[i].x(), actual[i].x()) << "object " << i;
        EXPECT_EQ(expected[i].y(), actual[i].y()) << "object " << i;
    }
}
}  // namespace

// Gravity is straight down and every object is identical, so an object only ever leaves its
// starting column by being pushed out of one, and the whole grid only spreads wider than it
// started through collisions. Both are what the passes have to reproduce.
TEST(VerletSolverTest, ObjectsCollide)  // NOLINT
{
    const auto initial = Simulate(1, 0);
    const auto simulated = Simulate(1, kSteps);
    ASSERT_EQ(initial.size(), simulated.size());

    size_t moved_sideways = 0;
    float initial_width = 0, simulated_width = 0;
    for (size_t i = 0; i != initial.size(); ++i)
    {
        if (std::abs(initial[i].x() - simulated[i].x()) > 0.f) ++moved_sideways;
        initial_width = std::max(initial_width, std::abs(initial[i].x() - initial.front().x()));
        simulated_width = std::max(simulated_width, std::abs(simulated[i].x() - initial.front().x()));
    }

    EXPECT_GT(moved_sideways, initial.size() / 2);
    EXPECT_GT(simulated_width, initial_width);
}

TEST(VerletSolverTest, RepeatedRunsMatch)  // NOLINT
{
    ExpectSamePositions(Simulate(4, kSteps), Simulate(4, kSteps));
}

TEST(VerletSolverTest, ResultIsThreadCountIndependent)  // NOLINT
{
    const auto single_threaded = Simulate(1, kSteps);
    for (const size_t threads_count : {size_t{2}, size_t{3}, size_t{4}, size_t{8}, size_t{16}})
    {
        SCOPED_TRACE(threads_count);
        ExpectSamePositions(single_threaded, Simulate(threads_count, kSteps));
    }
}
