#pragma once

#include <CppReflection/GetTypeInfo.hpp>

#include "CppReflection/Type.hpp"
#include "ankerl/unordered_dense.h"

namespace klgl
{

class IEventListener
{
public:
    using CallbackFunction = void (*)(IEventListener* listener, const void* event_data);

    virtual ~IEventListener() = default;
    virtual std::vector<const cppreflection::Type*> GetEventTypes() const = 0;
    virtual CallbackFunction MakeCallbackFunction(const size_t index) = 0;
};

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

// namespace event_detail
// {

// template <typename Signature>
// struct FunctionSignature
// {
//     static constexpr bool is_function = false;
// };

// template <typename Return_, typename Object_, typename... Args_>
// struct FunctionSignature<Return_ (Object_::*)(Args_...)>
// {
//     static constexpr bool is_function = true;
//     static constexpr bool is_stateless = false;
//     using Object = Object_;
//     using Return = Return_;
//     using Args = std::tuple<Args_...>;

//     [[nodiscard]] static constexpr size_t NumArgs() { return sizeof...(Args_); }
// };

// template <typename T>
// concept FunctionOrMethod = FunctionSignature<T>::is_function;

// template <FunctionOrMethod Type>
// struct SignaturePack
// {
//     using CommonObjectType = typename FunctionSignature<Type>::Object;
//     static constexpr bool has_common_object_type = false;
// };

// template <FunctionOrMethod... Types>
//     requires(sizeof...(Types))
// struct SignaturesPack
// {
// };

// }  // namespace event_detail

// template <auto... methods>
//     requires(
//         sizeof...(methods) > 0 && (event_detail::FunctionSignature<decltype(methods)>::is_function && ... && true) &&
//         ((event_detail::FunctionSignature<decltype(methods)>::NumArgs() == 1) && ... && true))
// class EventListenerMethodCallbacks : public IEventListener
// {
// public:
//     static constexpr size_t kEventsCount = sizeof...(EventTypes);

//     std::vector<const cppreflection::Type*> GetEventTypes() const override
//     {
//         return {cppreflection::GetTypeInfo<EventTypes>()...};
//     }

//     static EventListenerMethodCallbacks FromFunctions(std::function<void(const EventTypes&)>... functions)
//     {
//         EventListenerMethodCallbacks result{};
//         result.typed_callbacks_ = {std::move(functions)...};
//         result.InitializeWrappers();
//         return result;
//     }

//     static std::unique_ptr<EventListener> PtrFromFunctions(std::function<void(const EventTypes&)>... functions)
//     {
//         auto ptr = std::make_unique<EventListener>();
//         *ptr = FromFunctions(std::move(functions)...);
//         return ptr;
//     }

//     CallbackFunction MakeCallbackFunction(const size_t index) override { return wrappers_[index]; }

// private:
//     template <size_t index>
//     static void Callback(IEventListener* listener, const void* event_data)
//     {
//         using EventType = std::tuple_element_t<index, std::tuple<EventTypes...>>;
//         auto this_ = static_cast<EventListener*>(listener);
//         auto& event = *reinterpret_cast<const EventType*>(event_data);  // NOLINT
//         std::get<index>(this_->typed_callbacks_)(event);
//     }

//     void InitializeWrappers()
//     {
//         [&]<size_t... indices>(std::index_sequence<indices...>)
//         {
//             ((wrappers_[indices] = Callback<indices>), ...);
//         }
//         (std::make_index_sequence<kEventsCount>());
//     }

// private:
//     std::array<CallbackFunction, kEventsCount> wrappers_;
//     std::tuple<std::function<void(const EventTypes&)>...> typed_callbacks_;
// };

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

}  // namespace klgl
