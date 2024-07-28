#pragma once

#include "CppReflection/Type.hpp"
#include "klgl/memory/type_erased_array.hpp"

namespace klgl
{
class ReflectionUtils
{
public:
    [[nodiscard]] static klgl::TypeErasedArray MakeTypeErasedArray(const cppreflection::Type& type);
};
}  // namespace klgl
