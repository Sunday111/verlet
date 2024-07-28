#include "klgl/reflection/reflection_utils.hpp"

namespace klgl
{
[[nodiscard]] klgl::TypeErasedArray ReflectionUtils::MakeTypeErasedArray(const cppreflection::Type& type)
{
    return TypeErasedArray(TypeErasedArray::TypeInfo{
        .special_members = type.GetSpecialMembers(),
        .alignment = static_cast<uint32_t>(type.GetAlignment()),
        .object_size = static_cast<uint32_t>(type.GetInstanceSize()),
    });
}
}  // namespace klgl
