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

TEST(VerletSolverTest, RejectsSelfLinksAndMissingEndpoints)  // NOLINT
{
    verlet::VerletSolver solver;
    const auto a = std::get<0>(solver.objects.Alloc());
    const auto b = std::get<0>(solver.objects.Alloc());
    EXPECT_ANY_THROW(solver.CreateLink(a, a, 1.f));
    EXPECT_ANY_THROW(solver.CreateLink(a, verlet::kInvalidObjectId, 1.f));
    solver.DeleteObject(b);
    EXPECT_ANY_THROW(solver.CreateLink(a, b, 1.f));
    EXPECT_ANY_THROW(solver.CreateLink(b, a, 1.f));
    size_t links = 0;
    solver.ForEachLink([&](auto, auto, float) { ++links; });
    EXPECT_EQ(links, 0U);
}

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

TEST(VerletSolverTest, CoincidentLinksStayFiniteAndRespectMobility)  // NOLINT
{
    for (bool movable_a : {false, true})
    {
        for (bool movable_b : {false, true})
        {
            for (float separation : {0.f, 1e-30f, 0.25f})
            {
                SCOPED_TRACE(movable_a);
                SCOPED_TRACE(movable_b);
                SCOPED_TRACE(separation);
                verlet::VerletSolver solver;
                const auto a_id = std::get<0>(solver.objects.Alloc());
                const auto b_id = std::get<0>(solver.objects.Alloc());
                auto& a = solver.objects.Get(a_id);
                auto& b = solver.objects.Get(b_id);
                a.movable = movable_a;
                b.movable = movable_b;
                a.position = {separation, 0.f};
                solver.CreateLink(a_id, b_id, 2.f);

                solver.ApplyLinks();

                EXPECT_TRUE(a.position.IsFinite());
                EXPECT_TRUE(b.position.IsFinite());
                if (!movable_a) EXPECT_EQ(a.position, (edt::Vec2f{separation, 0.f}));
                if (!movable_b) EXPECT_EQ(b.position, edt::Vec2f{});
                if (movable_a || movable_b)
                {
                    EXPECT_FLOAT_EQ((a.position - b.position).Length(), 2.f);
                    EXPECT_GT(a.position.x(), b.position.x());
                }
            }
        }
    }
}

TEST(VerletSolverTest, BoundaryParticlesAlwaysReceivePhysicsUpdates)
{
    for (size_t threads : {size_t{1}, size_t{4}})
    {
        for (const edt::Vec2f position :
             {edt::Vec2f{-101.f, 0.f},
              {-100.f, 0.f},
              {-99.5f, 0.f},
              {101.f, 0.f},
              {100.f, 0.f},
              {0.f, -101.f},
              {0.f, -100.f},
              {0.f, -99.5f},
              {0.f, 101.f},
              {0.f, 100.f}})
        {
            verlet::VerletSolver solver;
            solver.SetThreadsCount(threads);
            auto [id, object] = solver.objects.Alloc();
            std::ignore = id;
            object.position = object.old_position = position;
            object.movable = true;
            solver.RebuildGrid();
            for (size_t thread = 0; thread != threads; ++thread) solver.UpdatePositions(thread, threads);
            const auto expected = solver.GetSimArea().Enlarged(-2.f).Clamp(
                position + solver.gravity * edt::Math::Sqr(solver.kTimeSubStepDurationSeconds));
            EXPECT_EQ(object.position, expected);
            EXPECT_EQ(object.old_position, position);
        }
    }
}

TEST(VerletSolverTest, GridPaddingContainsBoundaryCoordinates)
{
    for (float extent : {0.f, 0.5f, 1.f, 3.5f, 200.f})
    {
        verlet::VerletSolver solver;
        solver.SetThreadsCount(1);
        solver.SetSimArea({.x = {.begin = 0.f, .end = extent}, .y = {.begin = 0.f, .end = extent}});
        solver.RebuildGrid();
        const size_t last = std::max(size_t{1}, static_cast<size_t>(extent));
        EXPECT_EQ(solver.LocationToCell({-1.f, -1.f}), (edt::Vec2<size_t>{1, 1}));
        EXPECT_EQ(solver.LocationToCell({0.f, 0.f}), (edt::Vec2<size_t>{1, 1}));
        EXPECT_EQ(solver.LocationToCell({extent, extent}), (edt::Vec2<size_t>{last, last}));
        EXPECT_EQ(solver.LocationToCell({extent + 1.f, extent + 1.f}), (edt::Vec2<size_t>{last, last}));
        EXPECT_EQ(solver.GetGridCellsCount(), (last + 2) * (last + 2));
    }
}

TEST(VerletSolverTest, InteriorCellIdentifiersRemainStable)
{
    verlet::VerletSolver solver;
    solver.SetThreadsCount(1);
    EXPECT_EQ(solver.LocationToCell({-98.f, -97.f}), (edt::Vec2<size_t>{2, 3}));
    EXPECT_EQ(solver.LocationToCell({0.f, 0.f}), (edt::Vec2<size_t>{100, 100}));
    EXPECT_EQ(solver.LocationToCell({99.5f, 99.5f}), (edt::Vec2<size_t>{199, 199}));
    EXPECT_EQ(solver.LocationToCell({100.f, 100.f}), (edt::Vec2<size_t>{200, 200}));

    for (const edt::Vec2f minimum : {edt::Vec2f{0.3f, -0.3f}, {-20.75f, 10.25f}, {1'000'000.f, -1'000'000.f}})
    {
        const auto area = edt::FloatRange2Df::FromMinMax(minimum, minimum + edt::Vec2f{16.f, 32.f});
        solver.SetSimArea(area);
        for (size_t x = 1; x < 16; ++x)
        {
            for (size_t y = 1; y < 32; ++y)
            {
                const auto position =
                    minimum + edt::Vec2f{static_cast<float>(x) + 0.25f, static_cast<float>(y) + 0.25f};
                EXPECT_EQ(solver.LocationToCell(position), (edt::Vec2<size_t>{x, y}));
            }
        }
    }
}

TEST(VerletSolverTest, AreaChangesImmediatelyRefreshCellMapping)
{
    verlet::VerletSolver solver;
    solver.SetThreadsCount(1);
    solver.RebuildGrid();
    solver.SetSimArea(edt::FloatRange2Df::FromMinMax({0.3f, 0.3f}, {10.8f, 10.8f}));
    EXPECT_EQ(solver.LocationToCell({-100.f, -100.f}), (edt::Vec2<size_t>{1, 1}));
    EXPECT_EQ(solver.LocationToCell({1.3f, 1.3f}), (edt::Vec2<size_t>{1, 1}));
    EXPECT_EQ(solver.LocationToCell({5.3f, 5.3f}), (edt::Vec2<size_t>{5, 5}));
    EXPECT_EQ(solver.LocationToCell({100.f, 100.f}), (edt::Vec2<size_t>{10, 10}));

    const auto resized = edt::FloatRange2Df::FromMinMax({20.3f, -40.25f}, {27.8f, -30.5f});
    solver.SetSimArea(resized);
    EXPECT_EQ(solver.LocationToCell(resized.Min()), (edt::Vec2<size_t>{1, 1}));
    EXPECT_EQ(solver.LocationToCell(resized.Min() + edt::Vec2f{3.5f, 4.5f}), (edt::Vec2<size_t>{3, 4}));
    EXPECT_EQ(solver.LocationToCell(resized.Max()), (edt::Vec2<size_t>{7, 9}));
    solver.RebuildGrid();
    EXPECT_EQ(solver.GetGridCellsCount(), 9U * 11U);

    solver.SetSimArea(resized);
    EXPECT_EQ(solver.LocationToCell(resized.Max()), (edt::Vec2<size_t>{7, 9}));
    solver.SetSimArea(edt::FloatRange2Df::FromMinMax({-100.f, -100.f}, {100.f, 100.f}));
    EXPECT_EQ(solver.LocationToCell({0.f, 0.f}), (edt::Vec2<size_t>{100, 100}));
}
