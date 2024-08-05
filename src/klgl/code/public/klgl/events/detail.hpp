#pragma once

#include <cstddef>
#include <tuple>

namespace klgl::events::detail
{

template <typename Signature>
struct FunctionSignatureImpl
{
    [[nodiscard]] static constexpr bool IsFunction() { return false; }
};

template <typename Return_, typename Object_, typename... Args_>
struct FunctionSignatureImpl<Return_ (Object_::*)(Args_...)>
{
    using Object = std::decay_t<Object_>;
    using Return = Return_;
    using Args = std::tuple<Args_...>;

    [[nodiscard]] static constexpr bool IsFunction() { return true; }
    [[nodiscard]] static constexpr size_t NumArgs() { return sizeof...(Args_); }
};

template <typename Return_, typename Object_, typename... Args_>
struct FunctionSignatureImpl<Return_ (Object_::*)(Args_...) const>
{
    using Object = std::decay_t<Object_>;
    using Return = Return_;
    using Args = std::tuple<Args_...>;

    [[nodiscard]] static constexpr bool IsFunction() { return true; }
    [[nodiscard]] static constexpr size_t NumArgs() { return sizeof...(Args_); }
};

template <typename Return_, typename... Args_>
struct FunctionSignatureImpl<Return_ (*)(Args_...)>
{
    using Object = void;
    using Return = Return_;
    using Args = std::tuple<Args_...>;

    [[nodiscard]] static constexpr bool IsFunction() { return true; }
    [[nodiscard]] static constexpr size_t NumArgs() { return sizeof...(Args_); }
};

template <typename Signature>
using FunctionSignature = FunctionSignatureImpl<std::decay_t<Signature>>;

template <typename T>
concept FunctionOrMethod = FunctionSignature<T>::IsFunction();

template <FunctionOrMethod...>
struct SignaturesPack;

template <FunctionOrMethod Type>
struct SignaturesPack<Type>
{
    using CallableFromClass = typename FunctionSignature<Type>::Object;
    static constexpr bool callable_from_same_class = true;
};

template <FunctionOrMethod Head, FunctionOrMethod... Tail>
    requires(sizeof...(Tail) > 0)
struct SignaturesPack<Head, Tail...>
{
    using HeadObj = typename FunctionSignature<Head>::Object;
    using TailObj = typename SignaturesPack<Tail...>::CallableFromClass;
    static constexpr bool callable_from_same_class =
        std::same_as<HeadObj, void> || std::same_as<TailObj, void> || std::same_as<HeadObj, TailObj>;
    using CallableFromClass = std::conditional_t<std::same_as<HeadObj, void>, TailObj, HeadObj>;
};

template <typename T>
concept IsConstRef =
    std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>> && std::same_as<const std::decay_t<T>&, T>;
static_assert(IsConstRef<const int&>);

template <FunctionOrMethod T, size_t index>
using ArgByIndex = std::tuple_element_t<index, typename FunctionSignature<T>::Args>;

template <typename T>
concept ValidMethodEventListenerT = FunctionOrMethod<T> &&                     // Function or method pointer
                                    (FunctionSignature<T>::NumArgs() == 1) &&  // One const ref argument
                                    IsConstRef<ArgByIndex<T, 0>>;

template <typename... Callables>
concept ValidMethodEventListenersT = (sizeof...(Callables) > 0) &&                              // At least one event
                                     SignaturesPack<Callables...>::callable_from_same_class &&  // Same class
                                     (ValidMethodEventListenerT<Callables> && ... && true);

template <auto callable>
concept ValidMethodEventListener = ValidMethodEventListenerT<decltype(callable)>;

template <auto... callables>
concept ValidMethodEventListeners = ValidMethodEventListenersT<decltype(callables)...>;

}  // namespace klgl::events::detail
