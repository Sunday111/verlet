#pragma once

#include "matrix.hpp"

template <std::floating_point T>
class FloatRange
{
public:
    [[nodiscard]] constexpr T Extent() const
    {
        return end - begin;
    }

    [[nodiscard]] constexpr bool Contains(float v) const
    {
        return v >= begin && v <= end;
    }

    void EnlargeLeft(const float v)
    {
        begin -= v;
    }
    void EnlargeRight(const float v)
    {
        end += v;
    }
    void Enlarge(const float value)
    {
        EnlargeLeft(value);
        EnlargeRight(value);
    }

    T begin = std::numeric_limits<T>::lowest();
    T end = std::numeric_limits<T>::max();
};

template <std::floating_point T>
class FloatRange2D
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
    [[nodiscard]] constexpr Vec2f Extent() const noexcept
    {
        return Vec2f{{x.Extent(), y.Extent()}};
    }

    [[nodiscard]] constexpr Vec2f Min() const noexcept
    {
        return {x.begin, y.begin};
    }

    [[nodiscard]] constexpr Vec2f Uniform(const float v) const noexcept
    {
        return Uniform(Vec2f{v, v});
    }

    [[nodiscard]] constexpr Vec2f Uniform(const Vec2f& v) const noexcept
    {
        return Min() + v * Extent();
    }

    void Enlarge(const float value)
    {
        x.Enlarge(value);
        y.Enlarge(value);
    }

    FloatRange<T> x{};
    FloatRange<T> y{};
};
