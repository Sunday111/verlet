#pragma once

#include <nlohmann/json_fwd.hpp>
#include <span>
#include <string>
#include <vector>

#include "EverydayTools/GUID.hpp"
#include "klgl/name_cache/name.hpp"

namespace klgl
{

class ShaderDefine
{
public:
    ShaderDefine(const ShaderDefine&) = delete;
    ShaderDefine(ShaderDefine&& another) noexcept;
    void MoveFrom(ShaderDefine& another);
    ShaderDefine& operator=(const ShaderDefine&) = delete;
    ShaderDefine& operator=(ShaderDefine&& another) noexcept;

    std::string GenDefine() const;
    void SetValue(std::span<const uint8_t> value_view);

    static ShaderDefine ReadFromJson(const nlohmann::json& json);

protected:
    ShaderDefine() = default;

public:
    std::vector<uint8_t> value;
    Name name{};
    edt::GUID type_guid;
};

}  // namespace klgl
