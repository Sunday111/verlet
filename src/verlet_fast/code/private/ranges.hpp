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
}  // namespace verlet
