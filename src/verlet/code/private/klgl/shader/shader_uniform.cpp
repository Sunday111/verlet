#include "klgl/shader/shader_uniform.hpp"

#include <stdexcept>

#include "CppReflection/TypeRegistry.hpp"
#include "EverydayTools/GUID_fmtlib.hpp"
#include "fmt/core.h"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/reflection/matrix_reflect.hpp"
#include "klgl/shader/sampler_uniform.hpp"

namespace klgl
{

template <typename T>
struct ValueTypeHelper
{
    static bool Exec(edt::GUID type_guid, uint32_t location, std::span<const uint8_t> value)
    {
        if (cppreflection::GetStaticTypeInfo<T>().guid == type_guid)
        {
            OpenGl::SetUniform(location, *reinterpret_cast<const T*>(value.data()));  // NOLINT
            return true;
        }

        return false;
    }
};

template <>
struct ValueTypeHelper<SamplerUniform>
{
    static bool
    Exec(edt::GUID type_guid, [[maybe_unused]] uint32_t location, [[maybe_unused]] std::span<const uint8_t> value)
    {
        if (cppreflection::GetStaticTypeInfo<SamplerUniform>().guid == type_guid)
        {
            auto& v = *reinterpret_cast<const SamplerUniform*>(value.data());  // NOLINT
            static_assert(GL_TEXTURE31 - GL_TEXTURE0 == 31);
            glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + v.sampler_index));
            glBindTexture(GL_TEXTURE_2D, v.texture);
            glUniform1i(static_cast<GLint>(location), static_cast<GLint>(v.sampler_index));

            return true;
        }

        return false;
    }
};

template <typename T>
bool SendActualValue(edt::GUID type_guid, uint32_t location, std::span<const uint8_t> value)
{
    return ValueTypeHelper<T>::Exec(type_guid, location, value);
}

ShaderUniform::ShaderUniform() = default;

ShaderUniform::~ShaderUniform()
{
    Clear();
}

ShaderUniform::ShaderUniform(ShaderUniform&& another) noexcept
{
    MoveFrom(another);
}

void ShaderUniform::MoveFrom(ShaderUniform& another)
{
    Clear();
    name_ = another.name_;
    value_ = std::move(another.value_);
    location_ = another.location_;
    type_guid_ = another.type_guid_;
    sent_ = another.sent_;
}

void ShaderUniform::SendValue() const
{
    CheckNotEmpty();

    if (sent_)
    {
        return;
    }

    sent_ = true;

    const bool type_found = SendActualValue<float>(type_guid_, location_, value_) ||
                            SendActualValue<Vec2f>(type_guid_, location_, value_) ||
                            SendActualValue<Vec3f>(type_guid_, location_, value_) ||
                            SendActualValue<Vec4f>(type_guid_, location_, value_) ||
                            SendActualValue<Mat3f>(type_guid_, location_, value_) ||
                            SendActualValue<Mat4f>(type_guid_, location_, value_) ||
                            SendActualValue<SamplerUniform>(type_guid_, location_, value_);

    [[unlikely]] if (!type_found)
    {
        const cppreflection::Type* type_info = cppreflection::GetTypeRegistry()->FindType(type_guid_);
        throw std::runtime_error(fmt::format("Invalid type {}", type_info->GetName()));
    }
}

void ShaderUniform::SetType(edt::GUID type_guid)
{
    Clear();
    auto type_info = cppreflection::GetTypeRegistry()->FindType(type_guid);
    [[unlikely]] if (!type_info)
    {
        auto error = fmt::format("Unknown type id {}\n", type_guid);
        throw std::runtime_error(error);
    }

    type_guid_ = type_guid;
    value_.resize(type_info->GetInstanceSize());
    type_info->GetSpecialMembers().defaultConstructor(value_.data());
}

void ShaderUniform::EnsureTypeMatch(edt::GUID type_guid) const
{
    [[unlikely]] if (GetTypeGUID() != type_guid)
    {
        throw std::runtime_error("Trying to assign a value of invalid type to uniform");
    }
}

bool ShaderUniform::IsEmpty() const noexcept
{
    return value_.empty();
}

void ShaderUniform::SetValue(std::span<const uint8_t> value)
{
    CheckNotEmpty();
    sent_ = false;
    const cppreflection::Type* type_info = cppreflection::GetTypeRegistry()->FindType(type_guid_);
    assert(type_info->GetInstanceSize() == value.size());
    type_info->GetSpecialMembers().copyAssign(value_.data(), value.data());
}

ShaderUniform& ShaderUniform::operator=(ShaderUniform&& another) noexcept
{
    MoveFrom(another);
    return *this;
}

void ShaderUniform::Clear()
{
    if (!IsEmpty())
    {
        const cppreflection::Type* type_info = cppreflection::GetTypeRegistry()->FindType(type_guid_);
        type_info->GetSpecialMembers().destructor(value_.data());
        value_.clear();
        sent_ = false;
    }
}

void ShaderUniform::CheckNotEmpty() const
{
    [[unlikely]] if (IsEmpty())
    {
        throw std::logic_error("Trying to use empty uniform");
    }
}

}  // namespace klgl
