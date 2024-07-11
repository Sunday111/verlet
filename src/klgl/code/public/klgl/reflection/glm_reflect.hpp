#pragma once

#include "CppReflection/ReflectionProvider.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "klgl/wrap/wrap_glm.hpp"

namespace cppreflection
{

template <>
struct TypeReflectionProvider<glm::vec4>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<glm::vec4>(
            "glm::vec4",
            edt::GUID::Create("1D2F4DA1-5416-4087-8543-820902BBACB2"));
    }
};

template <>
struct TypeReflectionProvider<glm::vec3>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<glm::vec3>(
            "glm::vec3",
            edt::GUID::Create("7AEE11B0-DCCB-4AFC-AD00-5B8EA4A0E015"));
    }
};

template <>
struct TypeReflectionProvider<glm::vec2>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<glm::vec2>(
            "glm::vec2",
            edt::GUID::Create("B01CA829-0E80-4CD9-95E9-D7F32266F093"));
    }
};

template <>
struct TypeReflectionProvider<glm::mat4>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<glm::mat4>(
            "glm::mat4",
            edt::GUID::Create("9B24C2C7-29CD-45F6-AC74-880866D492D0"));
    }
};

template <>
struct TypeReflectionProvider<glm::mat3>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<glm::mat3>(
            "glm::mat3",
            edt::GUID::Create("2D13324E-08A3-47AD-8AC8-9E861EF9F104"));
    }
};

}  // namespace cppreflection