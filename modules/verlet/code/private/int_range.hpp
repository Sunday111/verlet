#pragma once

#include <concepts>
#include <limits>
#include <tuple>

template <std::integral T>
class IntRange
{
public:
    [[nodiscard]] constexpr bool Contains(const T& value) const noexcept
    {
        return InRange(value, begin, end);
    }

    T begin = std::numeric_limits<T>::lowest();
    T end = std::numeric_limits<T>::max();
};

template <std::integral T>
class IntRange2D
{
public:
    [[nodiscard]] constexpr bool Contains(const std::tuple<T, T>& p) const noexcept
    {
        return x.Contains(std::get<0>(p)) && y.Contains(std::get<1>(p));
    }
    [[nodiscard]] constexpr bool Contains(const T& vx, const T& vy) const noexcept
    {
        return x.Contains(vx) && y.Contains(vy);
    }

    IntRange<T> x{};
    IntRange<T> y{};
};
