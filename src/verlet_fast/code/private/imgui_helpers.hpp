#pragma once

#include <cassert>
#include <concepts>
#include <limits>

#include "imgui.h"

namespace verlet
{
class ImGuiHelper
{
public:
    template <std::unsigned_integral T>
    static bool SliderUInt(const char* name, T* ptr, const T min, const T max)
    {
        static_assert(sizeof(T) <= sizeof(size_t));
        [[maybe_unused]] constexpr auto maxint = static_cast<size_t>(std::numeric_limits<int>::lowest());
        assert(min < maxint && max <= maxint);
        int value = static_cast<int>(*ptr);
        if (ImGui::SliderInt(name, &value, static_cast<int>(min), static_cast<int>(max)))
        {
            *ptr = static_cast<T>(value);
            return true;
        }

        return false;
    }
};
}  // namespace verlet
