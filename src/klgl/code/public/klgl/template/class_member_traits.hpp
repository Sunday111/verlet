#pragma once

#include <tuple>

namespace klgl
{

template <typename Member>
struct ClassMemberTraits
{
    static constexpr bool IsVariable() noexcept { return false; }
    static constexpr bool IsFunction() noexcept { return false; }
};

template <typename InClass, typename InMember>
struct ClassMemberTraits<InMember InClass::*>
{
    using Class = InClass;
    using Member = InMember;
    static constexpr bool IsVariable() noexcept { return true; }
    static constexpr bool IsFunction() noexcept { return false; }
};

template <typename InReturnType, typename InClass, typename... InArguments>
struct ClassMemberTraits<InReturnType (InClass::*)(InArguments...)>
{
    using Class = InClass;
    using ReturnType = InReturnType;
    using Arguments = std::tuple<InArguments...>;
    static constexpr bool IsVariable() noexcept { return false; }
    static constexpr bool IsFunction() noexcept { return true; }
};

}  // namespace klgl
