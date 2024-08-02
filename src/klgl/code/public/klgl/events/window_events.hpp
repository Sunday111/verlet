#pragma once

#include "CppReflection/ReflectionProvider.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "EverydayTools/Math/Matrix.hpp"

namespace klgl::events
{
class OnWindowResize
{
public:
    edt::Vec2i previous{};
    edt::Vec2i current{};
};

class OnMouseMove
{
public:
    edt::Vec2f previous{};
    edt::Vec2f current{};
};

class OnMouseScroll
{
public:
    edt::Vec2f value{};
};
}  // namespace klgl::events

namespace cppreflection
{

template <>
struct TypeReflectionProvider<klgl::events::OnWindowResize>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<klgl::events::OnWindowResize>(
            "OnWindowResize",
            edt::GUID::Create("24DC2E34-B85B-4772-A05B-09B4DD84497A"));
    }
};

template <>
struct TypeReflectionProvider<klgl::events::OnMouseMove>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<klgl::events::OnMouseMove>(
            "OnMouseMove",
            edt::GUID::Create("92FDFAB7-0D48-44A0-B0A3-9C2FA3EE9E68"));
    }
};

template <>
struct TypeReflectionProvider<klgl::events::OnMouseScroll>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<klgl::events::OnMouseScroll>(
            "OnMouseScroll",
            edt::GUID::Create("14FD5774-D251-49E4-92CC-8134242E266A"));
    }
};

}  // namespace cppreflection
