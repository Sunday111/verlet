#pragma once

#include <CppReflection/GetTypeInfo.hpp>

#include "CppReflection/Type.hpp"
#include "ankerl/unordered_dense.h"

namespace klgl
{

class IEventListener
{
public:
    using CallbackFunction =
        void (*)(const cppreflection::Type* type, IEventListener* listener, const void* event_data);

    virtual ~IEventListener() = default;
    virtual std::vector<const cppreflection::Type*> GetEventTypes() const = 0;
    virtual CallbackFunction MakeCallbackFunction(const cppreflection::Type* type) = 0;
};

template <typename EventType>
class EventListener final : public IEventListener
{
public:
    std::vector<const cppreflection::Type*> GetEventTypes() const override
    {
        return {cppreflection::GetTypeInfo<EventType>()};
    }

    CallbackFunction MakeCallbackFunction([[maybe_unused]] const cppreflection::Type* type) override
    {
        assert(type == cppreflection::GetTypeInfo<EventType>());
        return []([[maybe_unused]] const cppreflection::Type* type, IEventListener* listener, const void* event_data)
        {
            auto this_ = reinterpret_cast<const EventListener*>(listener);  // NOLINT
            auto& event = *reinterpret_cast<const EventType*>(event_data);  // NOLINT
            this_->callback(event);
        };
    }

    std::function<void(EventType)> callback;
};

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
    [[nodiscard("Use return value to remove event listener")]] IEventListener* AddEventListener(
        IEventListener& listener);

    void RemoveListener(IEventListener* listener);
    void UpdateListenTypes(IEventListener* listener);

    template <typename EventType>
    void Emit(const EventType& event)
    {
        Emit(cppreflection::GetTypeInfo<EventType>(), &event, EventListener<EventType>::Invoke);
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

    ankerl::unordered_dense::set<std::unique_ptr<IEventListener>, PtrHasher, std::equal_to<>> owned_listeners_;
};

}  // namespace klgl
