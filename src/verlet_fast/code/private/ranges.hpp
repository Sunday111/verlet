#pragma once

#include <ranges>

namespace verlet
{
template <std::ranges::random_access_range Range>
[[nodiscard]] constexpr inline auto RangeIndices(const Range& range)
{
    return std::views::iota(0uz, std::ranges::size(range));
}

template <std::ranges::random_access_range Range>
[[nodiscard]] constexpr inline auto Enumerate(Range& range)
{
    return RangeIndices(range) | std::views::transform(
                                     [&](const auto& index)
                                     {
                                         using IndexType = std::decay_t<decltype(index)>;
                                         using ValueType = decltype(std::declval<Range&>()[0]);
                                         return std::tuple<IndexType, const ValueType&>{index, range[index]};
                                     });
}

template <typename T>
T vmin(T&& t)
{
    return std::forward<T>(t);
}

template <typename T0, typename T1, typename... Ts>
T0 vmin(T0&& val1, T1&& val2, Ts&&... vs)
{
    if (val2 < val1)
    {
        return vmin(val2, std::forward<Ts>(vs)...);
    }
    else
    {
        return vmin(val1, std::forward<Ts>(vs)...);
    }
}

template <std::ranges::random_access_range... Range>
[[nodiscard]] constexpr inline auto Zip(Range&... range)
{
    return std::views::iota(0uz, vmin(range.size()...)) |
           std::views::transform([&](const auto& index) { return std::forward_as_tuple(range[index]...); });
}

}  // namespace verlet
