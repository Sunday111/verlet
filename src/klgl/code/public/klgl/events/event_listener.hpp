#pragma once

#include <CppReflection/GetTypeInfo.hpp>
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

    std::vector<const cppreflection::Type*> GetEventTypes() const override
    {
        return {cppreflection::GetTypeInfo<EventTypes>()...};
    }

    static EventListener FromFunctions(std::function<void(const EventTypes&)>... functions)
    {
        EventListener result{};
        result.typed_callbacks_ = {std::move(functions)...};
        result.InitializeWrappers();
        return result;
    }

    static std::unique_ptr<EventListener> PtrFromFunctions(std::function<void(const EventTypes&)>... functions)
    {
        auto ptr = std::make_unique<EventListener>();
        *ptr = FromFunctions(std::move(functions)...);
        return ptr;
    }

    CallbackFunction MakeCallbackFunction(const size_t index) override { return wrappers_[index]; }

private:
    template <size_t index>
    static void Callback(IEventListener* listener, const void* event_data)
    {
        using EventType = std::tuple_element_t<index, std::tuple<EventTypes...>>;
        auto this_ = static_cast<EventListener*>(listener);
        auto& event = *reinterpret_cast<const EventType*>(event_data);  // NOLINT
        std::get<index>(this_->typed_callbacks_)(event);
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
