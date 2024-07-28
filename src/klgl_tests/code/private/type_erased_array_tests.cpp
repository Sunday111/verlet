#include <random>
#include <ranges>

#include "array_action.hpp"
#include "fmt/core.h"
#include "gtest/gtest.h"
#include "klgl/memory/type_erased_array.hpp"
#include "klgl/memory/type_erased_array_adapter.hpp"

TEST(TypeErrasedArray, FuzzyTest)
{
    struct UsageStats
    {
        int32_t num_destroyed = 0;
    };

    struct TestStruct
    {
        int a = 82;
        float b = 123.45f;
        std::unique_ptr<UsageStats, decltype([](UsageStats* s) { s->num_destroyed++; })> c;
    };

    UsageStats usage_stats_actual{};
    auto array_actual = klgl::MakeTypeErasedArray<TestStruct>();
    auto adapter = klgl::MakeTypeErasedArrayAdapter<TestStruct>(array_actual);

    UsageStats usage_stats_expected{};
    std::vector<TestStruct> array_expected;

    auto same_at = [&](const size_t index)
    {
        auto& obj_actual = adapter[index];
        auto& obj_expected = array_expected[index];

        if (obj_actual.a != obj_expected.a)
        {
            fmt::println(
                "Different values for property 'a' at index {}. Expected: {}. Actual: {}.",
                index,
                obj_expected.a,
                obj_actual.a);
            return false;
        }

        if (std::bit_cast<uint32_t>(obj_actual.b) != std::bit_cast<uint32_t>(obj_expected.b))
        {
            fmt::println(
                "Different values for property 'b' at index {}. Expected: {}. Actual: {}.",
                index,
                obj_expected.b,
                obj_actual.b);
            return false;
        }

        if (std::bit_cast<uint32_t>(obj_actual.b) != std::bit_cast<uint32_t>(obj_expected.b))
        {
            fmt::println(
                "Different pointers for property 'c' at index {}. Expected: {}. Actual: {}.",
                index,
                static_cast<const void*>(obj_expected.c.get()),
                static_cast<const void*>(obj_actual.c.get()));
            return false;
        }

        return true;
    };

    auto all_same = [&]()
    {
        if (usage_stats_actual.num_destroyed != usage_stats_expected.num_destroyed)
        {
            fmt::println(
                "Different values for property 'num_destroyed'. Expected: {}. Actual: {}.",
                usage_stats_expected.num_destroyed,
                usage_stats_actual.num_destroyed);
            return false;
        }

        if (array_actual.Size() != array_expected.size())
        {
            return false;
        }

        return std::ranges::all_of(std::views::iota(size_t{0}, array_expected.size()), same_at);
    };

    ASSERT_TRUE(all_same());

    std::unordered_map<ArrayAction, std::function<void()>> action_to_fn;

    constexpr unsigned kSeed = 12345;
    std::mt19937 rnd(kSeed);  // NOLINT
    constexpr size_t kMaxSize = 10'000;
    std::uniform_int_distribution<size_t> size_distribution(0, kMaxSize);
    std::uniform_int_distribution<int> a_distr(0, 42);
    std::uniform_real_distribution<float> b_distr(0, 42);
    auto trace =
        []<typename... Args>([[maybe_unused]] fmt::format_string<Args...> format, [[maybe_unused]] Args&&... args)
    {
        constexpr bool kWithTraceLogs = false;
        if constexpr (kWithTraceLogs)
        {
            fmt::println(format, std::forward<Args>(args)...);
        }
    };

    auto init_index = [&](const size_t index)
    {
        auto& actual = adapter[index];
        auto& expected = array_expected[index];

        actual.a = a_distr(rnd);
        actual.b = b_distr(rnd);
        actual.c.reset(&usage_stats_actual);

        expected.a = actual.a;
        expected.b = actual.b;
        expected.c.reset(&usage_stats_expected);
    };

    action_to_fn[ArrayAction::Resize] = [&]
    {
        const size_t prev_size = array_expected.size();
        const size_t new_size = size_distribution(rnd);

        array_expected.resize(new_size);
        array_actual.Resize(new_size);
        trace("Resize({})", new_size);

        for (size_t i = prev_size; i < new_size; ++i)
        {
            init_index(i);
        }
    };

    action_to_fn[ArrayAction::Erase] = [&]
    {
        if (array_expected.empty()) return;

        const size_t index = rnd() % array_expected.size();
        trace("Erase({})", index);

        auto it = array_expected.begin();
        std::advance(it, index);
        array_expected.erase(it);

        array_actual.Erase(index);
    };

    [[maybe_unused]] auto print_array = [&]()
    {
        for (size_t i = 0; i != adapter.Size(); ++i)
        {
            fmt::println(
                "    {:>2} ({:>20}): a: {}, b: {}, c: {}",
                i,
                array_actual[i],
                adapter[i].a,
                adapter[i].b,
                static_cast<const void*>(adapter[i].c.get()));
        }
    };

    action_to_fn[ArrayAction::Insert] = [&]
    {
        if (array_expected.size() >= kMaxSize) return;

        size_t index = 0;
        if (const size_t size = array_expected.size(); size != 0)
        {
            index = rnd() % size;
        }
        trace("Insert({}) ", index);

        // fmt::println("  before:");
        // print_array();
        array_actual.Insert(index);
        // fmt::println("  after:");
        // print_array();

        auto it = array_expected.begin();
        std::advance(it, index);
        array_expected.insert(it, TestStruct{});

        init_index(index);
    };

    for (const ArrayAction action : GenerateRandomActions(rnd, 10000))
    {
        if (auto it = action_to_fn.find(action); it != action_to_fn.end())
        {
            it->second();
            ASSERT_TRUE(all_same());
        }
    }
}
