#include "klgl/events/event_manager.hpp"

#include "klgl/error_handling.hpp"

namespace klgl::events
{

IEventListener* EventManager::AddEventListener(std::unique_ptr<IEventListener> listener)
{
    auto [iterator, inserted] = owned_listeners_.insert(std::move(listener));
    klgl::ErrorHandling::Ensure(inserted, "Attempt to register the same listener twice!");
    return AddEventListener(*iterator->get());
}

IEventListener* EventManager::AddEventListener(IEventListener& listener)
{
    auto [iterator, inserted] = all_listeners_.try_emplace(&listener);
    klgl::ErrorHandling::Ensure(inserted, "Attempt to register the same listener twice!");
    UpdateListenTypes(&listener);
    return &listener;
}

void EventManager::UpdateListenTypes(IEventListener* listener)
{
    auto iterator = all_listeners_.find(listener);
    klgl::ErrorHandling::Ensure(
        iterator != all_listeners_.end(),
        "Attempt to update listener that was not added previously!");
    auto& listener_info = iterator->second;

    auto previous_types = listener_info.registered_types;

    {
        auto types = listener->GetEventTypes();
        for (size_t index = 0; index != types.size(); ++index)
        {
            if (auto type = types[index]; listener_info.registered_types.insert(type).second)
            {
                klgl::ErrorHandling::Ensure(type, "IEventListener::GetEventTypes returns nullptr!");
                auto callback = listener->MakeCallbackFunction(index);
                klgl::ErrorHandling::Ensure(callback, "IEventListener::MakeCallbackFunction returns nullptr!");
                type_lookup_[type].push_back({.listener = listener, .callback = callback});
            }
        }
    }

    for (const cppreflection::Type* type : previous_types)
    {
        if (!listener_info.registered_types.contains(type))
        {
            StopListeningEventType(listener, type);
        }
    }
}

void EventManager::RemoveListener(IEventListener* listener)
{
    klgl::ErrorHandling::Ensure(
        all_listeners_.contains(listener),
        "Attempt to remove listener that was not added previously!");

    {
        auto& info = all_listeners_[listener];
        for (const cppreflection::Type* type : info.registered_types)
        {
            StopListeningEventType(listener, type);
        }
    }

    all_listeners_.erase(listener);
    owned_listeners_.erase(listener);
}

void EventManager::Emit(const cppreflection::Type* event_type, const void* event_data)
{
    auto iterator = type_lookup_.find(event_type);
    if (iterator == type_lookup_.end()) return;

    auto& type_data = iterator->second;
    for (auto& entry : type_data)
    {
        entry.callback(entry.listener, event_data);
    }
}

void EventManager::StopListeningEventType(IEventListener* listener, const cppreflection::Type* type)
{
    if (auto it = type_lookup_.find(type); it != type_lookup_.end())
    {
        auto& entries = it->second;
        // Yes, it is stable algorithm because I want to preserve the order of event invocation
        const auto [erase_begin, erase_end] = std::ranges::remove(entries, listener, &ListenerTypeEntry::listener);
        entries.erase(erase_begin, erase_end);
    }
}
}  // namespace klgl::events
