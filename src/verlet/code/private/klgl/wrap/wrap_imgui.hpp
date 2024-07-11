#pragma once

#include "klgl/macro/warning_suppress.hpp"

// clang-format off
#ifdef __clang__
    #define include_imgui_begin \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        static_assert(true, "")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define include_imgui_begin \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"") \
        _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
        static_assert(true, "")
#elif defined(_MSC_VER)
    #define include_imgui_begin \
        warning_push \
        _Pragma("warning(disable : 4201)") \
        static_assert(true, "")
#endif

#define include_imgui_end warning_pop static_assert(true, "")
// clang-format on

include_imgui_begin;
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
include_imgui_end;

namespace klgl
{

template <typename T>
static constexpr ImGuiDataType_ CastDataType() noexcept
{
    if constexpr (std::is_same_v<T, int8_t>)
    {
        return ImGuiDataType_S8;
    }
    if constexpr (std::is_same_v<T, uint8_t>)
    {
        return ImGuiDataType_U8;
    }
    if constexpr (std::is_same_v<T, int16_t>)
    {
        return ImGuiDataType_S16;
    }
    if constexpr (std::is_same_v<T, uint16_t>)
    {
        return ImGuiDataType_U16;
    }
    if constexpr (std::is_same_v<T, int32_t>)
    {
        return ImGuiDataType_S32;
    }
    if constexpr (std::is_same_v<T, uint32_t>)
    {
        return ImGuiDataType_U32;
    }
    if constexpr (std::is_same_v<T, int64_t>)
    {
        return ImGuiDataType_S64;
    }
    if constexpr (std::is_same_v<T, uint64_t>)
    {
        return ImGuiDataType_U64;
    }
    if constexpr (std::is_same_v<T, float>)
    {
        return ImGuiDataType_Float;
    }
    if constexpr (std::is_same_v<T, double>)
    {
        return ImGuiDataType_Double;
    }
}

}  // namespace klgl
