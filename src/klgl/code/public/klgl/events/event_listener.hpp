#pragma once

#include <CppReflection/GetTypeInfo.hpp>
#include <concepts>
#include <functional>
#include <memory>

#include "event_listener_interface.hpp"

namespace klgl::events
{

template <typename... EventTypes>
    requires(sizeof...(EventTypes) > 0)
class EventListener final : public IEventListener
{
public:
    static constexpr size_t kEventsCount = sizeof...(EventTypes);

    template <size_t index>
    using EventTypeByIndex = std::tuple_element_t<index, std::tuple<EventTypes...>>;

    std::vector<const cppreflection::Type*> GetEventTypes() const override
    {
        return {cppreflection::GetTypeInfo<EventTypes>()...};
    }

    template <typename... Functors>
        requires(sizeof...(Functors) == kEventsCount && (std::invocable<Functors, EventTypes> && ... && true))
    static EventListener FromFunctions(Functors&&... functors)
    {
        EventListener result{};
        result.typed_callbacks_ = {std::forward<Functors>(functors)...};
        result.InitializeWrappers();
        return result;
    }

    template <typename... Functors>
        requires(sizeof...(Functors) == kEventsCount && (std::invocable<Functors, EventTypes> && ... && true))
    static std::unique_ptr<EventListener> PtrFromFunctions(Functors&&... functors)
    {
        auto ptr = std::make_unique<EventListener>();
        *ptr = FromFunctions(std::forward<Functors>(functors)...);
        return ptr;
    }

    CallbackFunction MakeCallbackFunction(const size_t index) override { return wrappers_[index]; }

private:
    template <size_t index>
    static void Callback(IEventListener* listener, const void* event_data)
    {
        auto this_ = static_cast<EventListener*>(listener);
        auto& event = *reinterpret_cast<const EventTypeByIndex<index>*>(event_data);  // NOLINT
        std::invoke(std::get<index>(this_->typed_callbacks_), event);
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
    std::tuple<std::function<void(const EventTypes&)>...> typed_callbacks_;
};
}  // namespace klgl::events
