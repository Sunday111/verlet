#pragma once

#include <cstddef>

#include "class_member_traits.hpp"

namespace klgl
{

template <auto Member>
[[nodiscard]] size_t MemberOffset() noexcept
{
    using MemberTraits = ClassMemberTraits<decltype(Member)>;
    using T = typename MemberTraits::Class;
    auto ptr = &(reinterpret_cast<T const volatile*>(NULL)->*Member);  // NOLINT
    return reinterpret_cast<size_t>(ptr);                              // NOLINT
}

}  // namespace klgl
