#pragma once

#include <type_traits>

namespace klgl
{
[[nodiscard]] inline constexpr auto GetEnumUnderlying(auto value) noexcept
{
    using U = std::underlying_type_t<decltype(value)>;
    return static_cast<U>(value);
}
}  // namespace klgl
