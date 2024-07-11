#include "klgl/shader/shader_define.hpp"

#include <stdexcept>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "EverydayTools/GUID_fmtlib.hpp"       // IWYU pragma: keep (formatter for guids)
#include "klgl/reflection/matrix_reflect.hpp"  // IWYU pragma: keep (provides reflection for matrices)
#include "nlohmann/json.hpp"

namespace klgl
{
using namespace edt::lazy_matrix_aliases;  // NOLINT

ShaderDefine::ShaderDefine(ShaderDefine&& another) noexcept
{
    MoveFrom(another);
}

ShaderDefine& ShaderDefine::operator=(ShaderDefine&& another) noexcept
{
    MoveFrom(another);
    return *this;
}

template <typename T>
inline static const T& CastBuffer(const std::vector<uint8_t>& buffer) noexcept
{
    assert(buffer.size() == sizeof(T));
    return *reinterpret_cast<const T*>(buffer.data());  // NOLINT
}

std::string ShaderDefine::GenDefine() const
{
    std::string value_str;
    if (type_guid == cppreflection::GetStaticTypeGUID<int>())
    {
        value_str = fmt::format("{}", CastBuffer<int>(value));
    }
    else if (type_guid == cppreflection::GetStaticTypeGUID<float>())
    {
        value_str = fmt::format("{}", CastBuffer<float>(value));
    }
    else if (type_guid == cppreflection::GetStaticTypeGUID<Vec3f>())
    {
        const auto& vec = CastBuffer<Vec3f>(value);
        value_str = fmt::format("vec3({}, {}, {})", vec.x(), vec.y(), vec.z());
    }

    return fmt::format("#define {} {}\n", name.GetView(), value_str);
}

void ShaderDefine::MoveFrom(ShaderDefine& another)
{
    name = another.name;
    type_guid = another.type_guid;
    value = std::move(another.value);
}

void ShaderDefine::SetValue(std::span<const uint8_t> value_view)
{
    auto type_registry = cppreflection::GetTypeRegistry();
    if (!type_registry->FindType(type_guid))
    {
        throw std::runtime_error(fmt::format("Unknown type guid {}", type_guid));
    }

    value.resize(value_view.size());
    std::copy(value_view.begin(), value_view.end(), value.begin());
}

template <typename T>
inline static std::span<const uint8_t> MakeValueSpan(const T& value) noexcept
{
    return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(T));  // NOLINT
}

ShaderDefine ShaderDefine::ReadFromJson(const nlohmann::json& json)
{
    ShaderDefine def;
    def.name = Name(json.at("name"));

    auto& default_value_json = json.at("default");
    std::string type_name = json.at("type");
    if (type_name == "float")
    {
        def.type_guid = cppreflection::GetStaticTypeGUID<float>();
        const float v = default_value_json;
        def.SetValue(MakeValueSpan(v));
    }
    else if (type_name == "int")
    {
        def.type_guid = cppreflection::GetStaticTypeGUID<int>();
        int v = default_value_json;
        def.SetValue(MakeValueSpan(v));
    }
    else if (type_name == "vec3")
    {
        def.type_guid = cppreflection::GetStaticTypeGUID<Vec3f>();
        Vec3f v;
        v.x() = default_value_json["x"];
        v.y() = default_value_json["y"];
        v.z() = default_value_json["z"];
        def.SetValue(MakeValueSpan(v));
    }
    else if (type_name == "vec2")
    {
        def.type_guid = cppreflection::GetStaticTypeGUID<Vec2f>();
        Vec2f v;
        v.x() = default_value_json["x"];
        v.y() = default_value_json["y"];
        def.SetValue(MakeValueSpan(v));
    }
    else
    {
        throw std::runtime_error(fmt::format("Unknown shader variable type: {}", type_name));
    }

    return def;
}

}  // namespace klgl
