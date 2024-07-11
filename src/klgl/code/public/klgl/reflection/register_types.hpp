#pragma once

#include "CppReflection/GetTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "klgl/reflection/matrix_reflect.hpp"  // IWYU pragma: keep (provides reflection for matrices)
#include "klgl/shader/sampler_uniform.hpp"

namespace klgl
{

inline void RegisterReflectionTypes()
{
    [[maybe_unused]] const cppreflection::Type* t{};
    t = cppreflection::GetTypeInfo<float>();
    t = cppreflection::GetTypeInfo<int8_t>();
    t = cppreflection::GetTypeInfo<int16_t>();
    t = cppreflection::GetTypeInfo<int32_t>();
    t = cppreflection::GetTypeInfo<int64_t>();
    t = cppreflection::GetTypeInfo<uint8_t>();
    t = cppreflection::GetTypeInfo<uint16_t>();
    t = cppreflection::GetTypeInfo<uint32_t>();
    t = cppreflection::GetTypeInfo<uint64_t>();
    t = cppreflection::GetTypeInfo<Vec3f>();
    t = cppreflection::GetTypeInfo<Vec4f>();
    t = cppreflection::GetTypeInfo<Mat3f>();
    t = cppreflection::GetTypeInfo<Mat4f>();
    t = cppreflection::GetTypeInfo<Vec2f>();
    t = cppreflection::GetTypeInfo<Vec3f>();
    t = cppreflection::GetTypeInfo<Vec4f>();
    t = cppreflection::GetTypeInfo<Mat3f>();
    t = cppreflection::GetTypeInfo<Mat4f>();
    t = cppreflection::GetTypeInfo<SamplerUniform>();
}

}  // namespace klgl
