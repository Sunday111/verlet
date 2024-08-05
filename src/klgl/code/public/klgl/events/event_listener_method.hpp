#pragma once

#include <memory>

#include "CppReflection/GetTypeInfo.hpp"
#include "detail.hpp"
#include "event_listener_interface.hpp"

namespace klgl::events
{

// This adapter over IEventListener provides a way to subscribe
// some class to a few different events easily and with minimal memory overhead.
// Template parameters are pointers to some class methods that will be called when event happens.
// Types of events for subscribtion will be deduced from specified methods signatures.
//
// For example, if you class has these methods:
//     void OnEvent1(const MyEvent1&);
//     void OnEvent2(const MyEvent2&);
// this listener can be created this way:
//     listener = klgl::events::EventListenerMethodCallbacks<
//          &MyClass::OnEvent1,
//          &MyClass::OnEvent2>::CreatePtr(this);
//     event_manager.AddEventListener(*listener_);
template <auto... methods>
    requires(detail::ValidMethodEventListeners<methods...>)
class EventListenerMethodCallbacks : public IEventListener
{
public:
    using EventTypesTuple = std::tuple<detail::ArgByIndex<decltype(methods), 0>...>;
    using ObjectType = detail::SignaturesPack<decltype(methods)...>::CallableFromClass;

    static constexpr size_t kEventsCount = sizeof...(methods);

    std::vector<const cppreflection::Type*> GetEventTypes() const override
    {
        return {cppreflection::GetTypeInfo<std::decay_t<detail::ArgByIndex<decltype(methods), 0>>>()...};
    }

    static EventListenerMethodCallbacks Create(ObjectType* object)
    {
        EventListenerMethodCallbacks result{};
        result.object_ = object;
        result.InitializeWrappers();
        return result;
    }

    static std::unique_ptr<IEventListener> CreatePtr(ObjectType* object)
    {
        auto result = std::make_unique<EventListenerMethodCallbacks>();
        result->object_ = object;
        result->InitializeWrappers();
        return result;
    }

    CallbackFunction MakeCallbackFunction(const size_t index) override { return wrappers_[index]; }

private:
    template <size_t index>
    static void Callback(IEventListener* listener, const void* event_data)
    {
        using EventType = std::decay_t<std::tuple_element_t<index, EventTypesTuple>>;
        auto this_ = static_cast<EventListenerMethodCallbacks*>(listener);
        auto& event = *reinterpret_cast<const EventType*>(event_data);  // NOLINT
        static constexpr auto methods_tuple = std::make_tuple(methods...);
        std::invoke(std::get<index>(methods_tuple), this_->object_, event);
    }

    void InitializeWrappers()
    {
        [&]<size_t... indices>(std::index_sequence<indices...>)
        {
            ((wrappers_[indices] = Callback<indices>), ...);
        }
        (std::make_index_sequence<kEventsCount>());
    }

private:
    std::array<CallbackFunction, kEventsCount> wrappers_;
    ObjectType* object_ = nullptr;
};
}  // namespace klgl::events
