#include <random>
#include <ranges>

#include "CppReflection/GetTypeInfo.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "array_action.hpp"
#include "fmt/core.h"
#include "gtest/gtest.h"
#include "klgl/memory/type_erased_array.hpp"
#include "klgl/memory/type_erased_array_adapter.hpp"
#include "klgl/reflection/reflection_utils.hpp"

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

namespace klgl
{

TEST(TypeErasedArray, ResizeInsertErase)
{
    UsageStats usage_stats_actual{};
    auto array_actual = klgl::ReflectionUtils::MakeTypeErasedArray(*cppreflection::GetTypeInfo<TestStruct>());
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

template <typename SrcType, typename DstType>
    requires(std::is_copy_assignable_v<SrcType> && std::is_copy_assignable_v<DstType>) bool
CopyAssignGenericTest()
{
    constexpr unsigned kSeed = 12345;
    std::mt19937 rnd(kSeed);  // NOLINT

    // fill arrays with same size
    constexpr size_t kMaxSize = 50;
    std::uniform_int_distribution<int> value_distr(-99999, 99999);

    for (size_t dst_capacity = 0; dst_capacity <= kMaxSize; ++dst_capacity)
    {
        for (size_t dst_size = 0; dst_size <= kMaxSize; ++dst_size)
        {
            for (size_t src_size = 0; src_size != kMaxSize; ++src_size)
            {
                auto array_dst = klgl::TypeErasedArray::Create<DstType>();
                array_dst.Reserve(dst_capacity);
                array_dst.Resize(dst_size);

                auto array_src = klgl::TypeErasedArray::Create<SrcType>();
                array_src.Reserve(src_size);
                array_src.Resize(src_size);

                {
                    auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<DstType>(array_dst);
                    auto adapter_src = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_src);
                    for (size_t i = 0; i != std::max(src_size, dst_size); ++i)
                    {
                        if (i < dst_size)
                        {
                            adapter_dst[i] = DstType::Rnd(rnd);
                        }

                        if (i < src_size)
                        {
                            adapter_src[i] = SrcType::Rnd(rnd);
                        }
                    }
                }

                auto same_at = [&](const size_t index)
                {
                    auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_dst);
                    auto adapter_src = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_src);

                    if (adapter_dst[index] != adapter_src[index])
                    {
                        fmt::println("Values at index {} are different", index);
                        return false;
                    }

                    return true;
                };

                array_dst = array_src;

                if (array_dst.Size() != array_src.Size())
                {
                    fmt::println(
                        "Arrays have different sizes after assignment. Src size: {}. Dst size: {}",
                        array_src.Size(),
                        array_dst.Size());
                    return false;
                }

                if (!std::ranges::all_of(std::views::iota(size_t{0}, array_dst.Size()), same_at))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
struct CopyableType
{
    T value = 42;
    bool operator==(const CopyableType&) const = default;
    bool operator!=(const CopyableType&) const = default;
    static CopyableType Rnd(std::mt19937& rnd) { return {.value = static_cast<T>(rnd())}; }
};

TEST(TypeErasedArray, CopyAssignSameTypes)
{
    ASSERT_TRUE((CopyAssignGenericTest<CopyableType<int>, CopyableType<int>>()));
}

TEST(TypeErasedArray, CopyAssignDifferentTypes)
{
    ASSERT_TRUE((CopyAssignGenericTest<CopyableType<int>, CopyableType<float>>()));
}

template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
struct MovableType
{
    std::unique_ptr<T> value;
    bool operator==(const MovableType& other) const
    {
        if (!value && !other.value) return true;
        if (value && other.value && *value == *other.value) return true;
        return false;
    }
    bool operator!=(const MovableType&) const = default;

    MovableType Copy() const { return {.value = value ? std::make_unique<T>(*value) : nullptr}; }
    static MovableType Rnd(std::mt19937& rnd) { return {.value = std::make_unique<T>(static_cast<T>(rnd()))}; }
};

template <typename SrcType, typename DstType>
    requires(std::is_move_assignable_v<SrcType> && std::is_move_assignable_v<DstType>) bool
MoveAssignGenericTest()
{
    constexpr unsigned kSeed = 12345;
    std::mt19937 rnd(kSeed);  // NOLINT

    // fill arrays with same size
    constexpr size_t kMaxSize = 50;
    std::uniform_int_distribution<int> value_distr(-99999, 99999);

    std::vector<SrcType> src_copy;

    for (size_t dst_capacity = 0; dst_capacity <= kMaxSize; ++dst_capacity)
    {
        for (size_t dst_size = 0; dst_size <= kMaxSize; ++dst_size)
        {
            for (size_t src_size = 0; src_size != kMaxSize; ++src_size)
            {
                auto array_dst = klgl::TypeErasedArray::Create<DstType>();
                array_dst.Reserve(dst_capacity);
                array_dst.Resize(dst_size);

                auto array_src = klgl::TypeErasedArray::Create<SrcType>();
                array_src.Reserve(src_size);
                array_src.Resize(src_size);
                auto adapter_src = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_src);

                {
                    auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<DstType>(array_dst);
                    for (size_t i = 0; i != std::max(src_size, dst_size); ++i)
                    {
                        if (i < dst_size)
                        {
                            adapter_dst[i] = DstType::Rnd(rnd);
                        }

                        if (i < src_size)
                        {
                            adapter_src[i] = SrcType::Rnd(rnd);
                        }
                    }
                }

                auto same_at = [&](const size_t index)
                {
                    auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_dst);

                    if (adapter_dst[index] != src_copy[index])
                    {
                        fmt::println("Values at index {} are different", index);
                        return false;
                    }

                    return true;
                };

                src_copy.resize(array_src.Size());
                for (size_t i = 0; i != array_src.Size(); ++i)
                {
                    src_copy[i] = adapter_src[i].Copy();
                }

                array_dst = std::move(array_src);

                if (array_dst.Size() != src_copy.size())
                {
                    fmt::println(
                        "After move assignment destination array has different size from copy made beforehand."
                        "Expected size: {}. Actual size: {}",
                        src_copy.size(),
                        array_dst.Size());
                    return false;
                }

                if (!std::ranges::all_of(std::views::iota(size_t{0}, array_dst.Size()), same_at))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

TEST(TypeErasedArray, MoveAssignSameTypes)
{
    ASSERT_TRUE((MoveAssignGenericTest<MovableType<int>, MovableType<int>>()));
}

TEST(TypeErasedArray, MoveAssignDifferentTypes)
{
    ASSERT_TRUE((MoveAssignGenericTest<MovableType<int>, MovableType<float>>()));
}

template <typename SrcType, typename DstType>
    requires(std::is_copy_constructible_v<SrcType> && std::is_copy_constructible_v<DstType>) bool
CopyConstructGenericTest()
{
    constexpr unsigned kSeed = 12345;
    std::mt19937 rnd(kSeed);  // NOLINT

    // fill arrays with same size
    constexpr size_t kMaxSize = 50;
    std::uniform_int_distribution<int> value_distr(-99999, 99999);

    for (size_t src_size = 0; src_size != kMaxSize; ++src_size)
    {
        auto array_src = klgl::TypeErasedArray::Create<SrcType>();
        auto adapter_src = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_src);
        array_src.Reserve(src_size);
        array_src.Resize(src_size);

        for (size_t i = 0; i != src_size; ++i)
        {
            if (i < src_size)
            {
                adapter_src[i] = SrcType::Rnd(rnd);
            }
        }

        klgl::TypeErasedArray array_dst = array_src;
        auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_dst);

        auto same_at = [&](const size_t index)
        {
            if (adapter_dst[index] != adapter_src[index])
            {
                fmt::println("Values at index {} are different", index);
                return false;
            }

            return true;
        };

        array_dst = array_src;

        if (array_dst.Size() != array_src.Size())
        {
            fmt::println(
                "Arrays have different sizes after copy construction. Src size: {}. Dst size: {}",
                array_src.Size(),
                array_dst.Size());
            return false;
        }

        if (!std::ranges::all_of(std::views::iota(size_t{0}, array_dst.Size()), same_at))
        {
            return false;
        }
    }

    return true;
}

TEST(TypeErasedArray, CopyConstructorSameTypes)
{
    ASSERT_TRUE((CopyConstructGenericTest<CopyableType<int>, CopyableType<int>>()));
}

TEST(TypeErasedArray, CopyConstructorDifferentTypes)
{
    ASSERT_TRUE((CopyConstructGenericTest<CopyableType<int>, CopyableType<float>>()));
}

template <typename SrcType, typename DstType>
    requires(std::is_move_constructible_v<SrcType> && std::is_move_constructible_v<DstType>) bool
MoveConstructGenericTest()
{
    constexpr unsigned kSeed = 12345;
    std::mt19937 rnd(kSeed);  // NOLINT

    // fill arrays with same size
    constexpr size_t kMaxSize = 50;
    std::uniform_int_distribution<int> value_distr(-99999, 99999);
    std::vector<SrcType> src_copy;

    for (size_t src_size = 0; src_size != kMaxSize; ++src_size)
    {
        auto array_src = klgl::TypeErasedArray::Create<SrcType>();
        auto adapter_src = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_src);
        array_src.Reserve(src_size);
        array_src.Resize(src_size);

        {
            for (size_t i = 0; i != src_size; ++i)
            {
                if (i < src_size)
                {
                    adapter_src[i] = SrcType::Rnd(rnd);
                }
            }
        }

        // Save the copy before move
        src_copy.resize(array_src.Size());
        for (size_t i = 0; i != array_src.Size(); ++i)
        {
            src_copy[i] = adapter_src[i].Copy();
        }

        auto array_dst = std::move(array_src);
        auto adapter_dst = klgl::MakeTypeErasedArrayAdapter<SrcType>(array_dst);

        auto same_at = [&](const size_t index)
        {
            if (adapter_dst[index] != src_copy[index])
            {
                fmt::println("Values at index {} are different", index);
                return false;
            }

            return true;
        };

        if (array_dst.Size() != src_copy.size())
        {
            fmt::println(
                "After move construct the resulting array has unexpected size. Expected size: {}. Actual size: {}",
                src_copy.size(),
                array_dst.Size());
            return false;
        }

        if (!std::ranges::all_of(std::views::iota(size_t{0}, array_dst.Size()), same_at))
        {
            return false;
        }
    }

    return true;
}

TEST(TypeErasedArray, MoveConstructorSameTypes)
{
    ASSERT_TRUE((MoveConstructGenericTest<MovableType<int>, MovableType<int>>()));
}

TEST(TypeErasedArray, MoveConstructorDifferentTypes)
{
    ASSERT_TRUE((MoveConstructGenericTest<MovableType<int>, MovableType<float>>()));
}

TEST(TypeErasedArray, Experiment)
{
    // This is an array of strings now
    auto array = klgl::TypeErasedArray::Create<std::string>();

    // Since we know the contained type is string, an adaptor can be used to avoid delaing with cases
    {
        auto a = klgl::MakeTypeErasedArrayAdapter<std::string>(array);
        array.Resize(4);
        a[0] = "Hello";
        a[1] = ",";
        a[2] = "world";
        a[3] = "!";

        for (size_t i = 0; i != a.Size(); ++i)
        {
            fmt::print("{}", a[i]);
        }
        fmt::println("");
    }

    fmt::println("Number of bytes allocated for string array: {}", array.CapacityBytes());
    fmt::println("String array capacity: {}", array.Capacity());

    // Now convert this array to integer array
    array = klgl::TypeErasedArray::Create<int>();

    fmt::println("Int array size: {}", array.Size());
    fmt::println("Number of bytes allocated for int array: {}", array.CapacityBytes());
    fmt::println("Int array capacity: {}", array.Capacity());

    {
        auto a = klgl::MakeTypeErasedArrayAdapter<int>(array);
        array.Resize(6);
        a[0] = 4;
        a[1] = 8;
        a[2] = 15;
        a[3] = 16;
        a[4] = 23;
        a[5] = 42;

        for (size_t i = 0; i != a.Size(); ++i)
        {
            fmt::print("{} ", a[i]);
        }
        fmt::println("");
    }
}
}  // namespace klgl
