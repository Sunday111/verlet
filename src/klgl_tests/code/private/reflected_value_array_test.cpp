#include "klgl/reflection/reflected_value_array.hpp"

#include <random>
#include <ranges>

#include "CppReflection/ReflectionProvider.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "gtest/gtest.h"

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

enum class ArrayAction
{
    Resize,
    Erase,
    Insert,
};

namespace cppreflection
{

template <>
struct TypeReflectionProvider<TestStruct>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<TestStruct>(
            "TestStruct",
            edt::GUID::Create("E331A808-897F-4CE6-89C5-48E1BA021BA1"));
    }
};

}  // namespace cppreflection

static constexpr std::array kArrayActions{
    ArrayAction::Resize,
    ArrayAction::Erase,
    ArrayAction::Insert,
};

std::vector<ArrayAction> GenerateRandomActions(std::mt19937& rnd, size_t count)
{
    std::uniform_int_distribution<size_t> index_distribution(0, kArrayActions.size() - 1);
    std::vector<ArrayAction> actions;
    actions.reserve(count);
    std::generate_n(std::back_inserter(actions), count, [&]() { return kArrayActions[index_distribution(rnd)]; });
    return actions;
}

TEST(ReflectedValueArray, FuzzyTest)  // NOLINT
{
    UsageStats usage_stats_actual{};
    klgl::ReflectedValueArray array_actual(*cppreflection::GetTypeInfo<TestStruct>());
    klgl::ReflectedValueArrayAdapter<TestStruct, klgl::ReflectedValueArray> adapter(array_actual);

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
    std::uniform_int_distribution<size_t> size_distribution(0, 10000);

    auto init_index = [&](const size_t index)
    {
        auto& actual = adapter[index];
        auto& expected = array_expected[index];
        actual.c.reset(&usage_stats_actual);
        expected.c.reset(&usage_stats_expected);
    };

    action_to_fn[ArrayAction::Resize] = [&]
    {
        const size_t prev_size = array_expected.size();
        const size_t new_size = size_distribution(rnd);

        array_expected.resize(new_size);
        array_actual.Resize(new_size);

        for (size_t i = prev_size; i < new_size; ++i)
        {
            init_index(i);
        }
    };

    action_to_fn[ArrayAction::Erase] = [&]
    {
        if (array_expected.empty()) return;

        const size_t index = rnd() % array_expected.size();

        auto it = array_expected.begin();
        std::advance(it, index);
        array_expected.erase(it);

        array_actual.Erase(index);
    };

    action_to_fn[ArrayAction::Insert] = [&] {
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
