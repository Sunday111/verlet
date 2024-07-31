#pragma once

#include <CppReflection/GetTypeInfo.hpp>

#include "CppReflection/Type.hpp"
#include "ankerl/unordered_dense.h"
#include "klgl/events/event_listener_interface.hpp"

namespace klgl::events
{

class EventManager
{
public:
    struct ListenerInfo
    {
        ankerl::unordered_dense::set<const cppreflection::Type*> registered_types;
    };

    struct ListenerTypeEntry
    {
        IEventListener* listener;
        IEventListener::CallbackFunction callback;
    };

    void Emit(const cppreflection::Type* event_type, const void* event_data);

    // Registers event listeners and takes ownership on the object
    [[nodiscard("Use return value to remove event listener")]] IEventListener* AddEventListener(
        std::unique_ptr<IEventListener> listener);

    // Registers listener by raw pointer but it is caller's responsibility
    // to guarantee object lifetime until RemoveListener is called
    IEventListener* AddEventListener(IEventListener& listener);

    void RemoveListener(IEventListener* listener);
    void UpdateListenTypes(IEventListener* listener);

    template <typename EventType>
    void Emit(const EventType& event)
    {
        Emit(cppreflection::GetTypeInfo<EventType>(), &event);
    }

private:
    void StopListeningEventType(IEventListener* listener, const cppreflection::Type* type);

private:
    ankerl::unordered_dense::map<const cppreflection::Type*, std::vector<ListenerTypeEntry>> type_lookup_;
    ankerl::unordered_dense::map<IEventListener*, ListenerInfo> all_listeners_;

    struct PtrHasher
    {
        using is_transparent = void;  // enable heterogeneous overloads

        [[nodiscard]] auto operator()(const std::unique_ptr<IEventListener>& ptr) const noexcept -> uint64_t
        {
            return (*this)(ptr.get());
        }

        [[nodiscard]] auto operator()(const IEventListener* ptr) const noexcept -> uint64_t
        {
            return ankerl::unordered_dense::hash<size_t>{}(std::bit_cast<size_t>(ptr));
        }
    };

    struct PtrComparator
    {
        using is_transparent = void;  // enable heterogeneous overloads

        bool operator()(const std::unique_ptr<IEventListener>& a, const std::unique_ptr<IEventListener>& b)
            const noexcept
        {
            return a == b;
        }

        bool operator()(const IEventListener* a, const IEventListener* b) const noexcept { return a == b; }

        bool operator()(const std::unique_ptr<IEventListener>& a, const IEventListener* b) const noexcept
        {
            return a.get() == b;
        }

        bool operator()(const IEventListener* a, const std::unique_ptr<IEventListener>& b) const noexcept
        {
            return a == b.get();
        }
    };

    ankerl::unordered_dense::set<std::unique_ptr<IEventListener>, PtrHasher, PtrComparator> owned_listeners_;
};

}  // namespace klgl::events
