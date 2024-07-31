#include "gtest/gtest.h"
#include "klgl/events/event_listener.hpp"
#include "klgl/events/event_listener_method.hpp"
#include "klgl/events/event_manager.hpp"

namespace klgl::events
{
class TestEventA
{
public:
    size_t value = 0;
};

class TestEventB
{
public:
    int value = 0;
};
}  // namespace klgl::events

namespace cppreflection
{

template <>
struct TypeReflectionProvider<klgl::events::TestEventA>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<klgl::events::TestEventA>(
            "TestEventA",
            edt::GUID::Create("C63EBDCA-938B-4D87-B095-EC72D54EAFE5"));
    }
};

template <>
struct TypeReflectionProvider<klgl::events::TestEventB>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<klgl::events::TestEventB>(
            "TestEventB",
            edt::GUID::Create("11A8E3B6-397D-4D28-B3D3-AFE75BBB22C5"));
    }
};

}  // namespace cppreflection

namespace klgl::events
{

namespace detail
{
struct EventManagerHelpersTest
{
    using Self = EventManagerHelpersTest;

    void Method_(const int&);
    void ConstMethod_(const int&) const;
    static void StaticMethod_(const double&);
    static constexpr auto method = &Self::Method_;
    static constexpr auto const_method = &Self::ConstMethod_;
    static constexpr auto static_method = &Self::StaticMethod_;
    using Method = decltype(method);
    using ConstMethod = decltype(const_method);
    using StaticMethod = decltype(static_method);

    static_assert(SignaturesPack<Method>::callable_from_same_class);
    static_assert(std::same_as<SignaturesPack<Method>::CallableFromClass, Self>);

    static_assert(SignaturesPack<ConstMethod>::callable_from_same_class);
    static_assert(std::same_as<SignaturesPack<ConstMethod>::CallableFromClass, Self>);

    static_assert(SignaturesPack<StaticMethod>::callable_from_same_class);
    static_assert(std::same_as<SignaturesPack<StaticMethod>::CallableFromClass, void>);

    static_assert(SignaturesPack<Method, ConstMethod>::callable_from_same_class);
    static_assert(std::same_as<SignaturesPack<Method, ConstMethod>::CallableFromClass, Self>);

    static_assert(SignaturesPack<Method, ConstMethod, StaticMethod>::callable_from_same_class);
    static_assert(std::same_as<SignaturesPack<Method, ConstMethod, StaticMethod>::CallableFromClass, Self>);

    static_assert(ValidMethodEventListener<method>);
    static_assert(ValidMethodEventListener<const_method>);
    static_assert(ValidMethodEventListener<static_method>);
    static_assert(ValidMethodEventListeners<method, const_method, static_method>);
};
}  // namespace detail

TEST(EventManager, SimpleSubscriptionWithLambda)
{
    struct
    {
        size_t sum = 0;
        size_t trigger_count = 0;
    } stats_a;

    struct
    {
        int sum = 0;
        size_t trigger_count = 0;
    } stats_b;

    EventManager event_manager;

    auto emit_events = [&]
    {
        for (int i = 0; i != 10; ++i)
        {
            event_manager.Emit(TestEventA{.value = static_cast<size_t>(i)});
            event_manager.Emit(TestEventB{.value = i - 9});
        }
    };

    auto callback_a = [&](const TestEventA& a)
    {
        stats_a.sum += a.value;
        ++stats_a.trigger_count;
    };

    auto callback_b = [&](const TestEventB& b)
    {
        stats_b.sum += b.value;
        ++stats_b.trigger_count;
    };

    // listener a stored in unique_ptr
    auto listener_a = event_manager.AddEventListener(EventListener<TestEventA>::PtrFromFunctions(callback_a));
    EXPECT_ANY_THROW(listener_a = event_manager.AddEventListener(*listener_a));

    emit_events();

    ASSERT_EQ(stats_a.sum, 45);
    ASSERT_EQ(stats_a.trigger_count, 10);

    // listener b is allocated on stack
    auto listener_b = EventListener<TestEventB>::FromFunctions(callback_b);
    event_manager.AddEventListener(listener_b);
    EXPECT_ANY_THROW(event_manager.AddEventListener(listener_b));

    emit_events();
    ASSERT_EQ(stats_a.sum, 90);
    ASSERT_EQ(stats_a.trigger_count, 20);
    ASSERT_EQ(stats_b.sum, -45);
    ASSERT_EQ(stats_b.trigger_count, 10);

    // listener ab calls both callbacks
    auto listener_ab = EventListener<TestEventA, TestEventB>::FromFunctions(callback_a, callback_b);
    event_manager.AddEventListener(listener_ab);
    EXPECT_ANY_THROW(event_manager.AddEventListener(listener_ab));

    emit_events();
    ASSERT_EQ(stats_a.sum, 180);
    ASSERT_EQ(stats_a.trigger_count, 40);
    ASSERT_EQ(stats_b.sum, -135);
    ASSERT_EQ(stats_b.trigger_count, 30);

    // remove listeners one by one

    event_manager.RemoveListener(listener_a);
    emit_events();
    ASSERT_EQ(stats_a.sum, 225);
    ASSERT_EQ(stats_a.trigger_count, 50);
    ASSERT_EQ(stats_b.sum, -225);
    ASSERT_EQ(stats_b.trigger_count, 50);

    event_manager.RemoveListener(&listener_b);
    emit_events();
    ASSERT_EQ(stats_a.sum, 270);
    ASSERT_EQ(stats_a.trigger_count, 60);
    ASSERT_EQ(stats_b.sum, -270);
    ASSERT_EQ(stats_b.trigger_count, 60);

    event_manager.RemoveListener(&listener_ab);
    emit_events();
    ASSERT_EQ(stats_a.sum, 270);
    ASSERT_EQ(stats_a.trigger_count, 60);
    ASSERT_EQ(stats_b.sum, -270);
    ASSERT_EQ(stats_b.trigger_count, 60);

    EXPECT_ANY_THROW(event_manager.RemoveListener(listener_a));
    EXPECT_ANY_THROW(event_manager.RemoveListener(&listener_b));
    EXPECT_ANY_THROW(event_manager.RemoveListener(&listener_ab));
}

TEST(EventManager, MethodListener)
{
    EventManager event_manager;

    class SomeClass
    {
    public:
        void HandleEventA(const TestEventA&) { a_count++; }
        void HandleEventB(const TestEventB&) { b_count++; }

        size_t a_count = 0;
        size_t b_count = 0;
    };

    SomeClass some_object;

    auto listener =
        EventListenerMethodCallbacks<&SomeClass::HandleEventA, &SomeClass::HandleEventB>::Create(&some_object);
    event_manager.AddEventListener(listener);

    for (int i = 0; i != 10; ++i)
    {
        event_manager.Emit(TestEventA{.value = static_cast<size_t>(i)});
        if (i > 4)
        {
            event_manager.Emit(TestEventB{.value = i - 9});
        }
    }

    event_manager.RemoveListener(&listener);
    ASSERT_EQ(some_object.a_count, 10);
    ASSERT_EQ(some_object.b_count, 5);
}

}  // namespace klgl::events
