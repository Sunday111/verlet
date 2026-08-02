#include "verlet/object_pool.hpp"

#include <vector>

#include "gtest/gtest.h"

namespace
{
std::vector<verlet::ObjectId> Identifiers(const verlet::ObjectPool& pool)
{
    std::vector<verlet::ObjectId> ids;
    for (const verlet::ObjectId id : pool.Identifiers()) ids.push_back(id);
    return ids;
}
}  // namespace

TEST(ObjectPoolTest, StartsEmpty)  // NOLINT
{
    const verlet::ObjectPool pool;
    EXPECT_EQ(pool.ObjectsCount(), 0U);
    EXPECT_TRUE(Identifiers(pool).empty());
}

TEST(ObjectPoolTest, AllocReportsTheObject)  // NOLINT
{
    verlet::ObjectPool pool;
    const auto [id, object] = pool.Alloc();

    EXPECT_EQ(pool.ObjectsCount(), 1U);
    EXPECT_EQ(Identifiers(pool), std::vector{id});
}

TEST(ObjectPoolTest, AllocatedObjectsAreDistinct)  // NOLINT
{
    verlet::ObjectPool pool;
    const auto [a, object_a] = pool.Alloc();
    const auto [b, object_b] = pool.Alloc();

    EXPECT_NE(a, b);
    EXPECT_EQ(pool.ObjectsCount(), 2U);
    EXPECT_EQ(Identifiers(pool).size(), 2U);
}

// Writing through the reference Alloc returned has to be visible through the pool.
TEST(ObjectPoolTest, KeepsWhatWasWrittenThroughTheReference)  // NOLINT
{
    verlet::ObjectPool pool;
    const auto [id, object] = pool.Alloc();
    object.position = {3.f, 4.f};
    object.movable = true;

    const verlet::VerletObject& stored = pool.Get(id);
    EXPECT_EQ(stored.position.x(), 3.f);
    EXPECT_EQ(stored.position.y(), 4.f);
    EXPECT_TRUE(stored.IsMovable());
}

TEST(ObjectPoolTest, FreeDropsTheObject)  // NOLINT
{
    verlet::ObjectPool pool;
    const auto [kept, kept_object] = pool.Alloc();
    const auto [dropped, dropped_object] = pool.Alloc();

    pool.Free(dropped);

    EXPECT_EQ(pool.ObjectsCount(), 1U);
    EXPECT_EQ(Identifiers(pool), std::vector{kept});
}

// A freed slot goes on the free list, so the next allocation takes it back rather than
// growing the pool.
TEST(ObjectPoolTest, ReusesAFreedSlot)  // NOLINT
{
    verlet::ObjectPool pool;
    const auto [first, first_object] = pool.Alloc();
    pool.Free(first);

    const auto [second, second_object] = pool.Alloc();

    EXPECT_EQ(second, first);
    EXPECT_EQ(pool.ObjectsCount(), 1U);
}

TEST(ObjectPoolTest, ClearRemovesEverything)  // NOLINT
{
    verlet::ObjectPool pool;
    for (size_t i = 0; i != 8; ++i) [[maybe_unused]] const auto entry = pool.Alloc();

    pool.Clear();

    EXPECT_EQ(pool.ObjectsCount(), 0U);
    EXPECT_TRUE(Identifiers(pool).empty());
}

// Clearing a pool with holes in it must not trip over the free slots.
TEST(ObjectPoolTest, ClearsAPoolWithHoles)  // NOLINT
{
    verlet::ObjectPool pool;
    std::vector<verlet::ObjectId> ids;
    for (size_t i = 0; i != 6; ++i) ids.push_back(std::get<0>(pool.Alloc()));

    pool.Free(ids[1]);
    pool.Free(ids[4]);
    ASSERT_EQ(pool.ObjectsCount(), 4U);

    pool.Clear();
    EXPECT_EQ(pool.ObjectsCount(), 0U);
    EXPECT_TRUE(Identifiers(pool).empty());
}
