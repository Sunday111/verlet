#pragma once

#include "CppReflection/ReflectionProvider.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "EverydayTools/Math/Matrix.hpp"

namespace cppreflection
{

template <>
struct TypeReflectionProvider<edt::Vec2f>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<edt::Vec2f>(
            "Vec2f",
            edt::GUID::Create("5033D902-33BA-4E6E-8811-97208BD0CA54"));
    }
};

template <>
struct TypeReflectionProvider<edt::Vec3f>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<edt::Vec3f>(
            "Vec3f",
            edt::GUID::Create("D86FFB80-0BCC-4BFB-A1FC-53D04B4F275A"));
    }
};

template <>
struct TypeReflectionProvider<edt::Vec4f>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<edt::Vec4f>(
            "Vec4f",
            edt::GUID::Create("8E25D085-8055-4F93-A8EE-47C8920D0314"));
    }
};

template <>
struct TypeReflectionProvider<edt::Mat3f>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<edt::Mat3f>(
            "Mat3f",
            edt::GUID::Create("BA6B59C2-56A9-47DC-994C-B6EC1B70CD14"));
    }
};

template <>
struct TypeReflectionProvider<edt::Mat4f>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<edt::Mat4f>(
            "Mat4f",
            edt::GUID::Create("18CFED1A-AEEA-4CE3-ADD5-56E6953780F1"));
    }
};

}  // namespace cppreflection
