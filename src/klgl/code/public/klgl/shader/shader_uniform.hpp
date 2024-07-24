#pragma once

#include <span>
#include <vector>

#include "EverydayTools/GUID.hpp"
#include "klgl/name_cache/name.hpp"

namespace klgl
{

class ShaderUniform
{
public:
    ShaderUniform();
    ~ShaderUniform();
    ShaderUniform(const ShaderUniform&) = delete;
    ShaderUniform(ShaderUniform&& another) noexcept;
    void MoveFrom(ShaderUniform& another);
    void SendValue() const;
    void SetValue(std::span<const uint8_t> new_value);
    void SetType(edt::GUID type_guid);
    void SetName(Name name) { name_ = name; }
    void SetLocation(uint32_t location) { location_ = location; }
    void EnsureTypeMatch(edt::GUID type_guid) const;

    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] std::string_view GetNameView() { return GetName().GetView(); }

    [[nodiscard]] Name GetName() const noexcept { return name_; }
    [[nodiscard]] edt::GUID GetTypeGUID() const noexcept { return type_guid_; }
    [[nodiscard]] uint32_t GetLocation() const noexcept { return location_; }

    //[[nodiscard]] std::span<uint8_t> GetValue() noexcept { return value_; }
    [[nodiscard]] std::span<const uint8_t> GetValue() const noexcept { return value_; }

    ShaderUniform& operator=(const ShaderUniform&) = delete;
    ShaderUniform& operator=(ShaderUniform&& another) noexcept;

private:
    void Clear();
    void CheckNotEmpty() const;

private:
    std::vector<uint8_t> value_;
    Name name_;
    uint32_t location_{};
    edt::GUID type_guid_;
    mutable bool sent_ = false;
};

}  // namespace klgl
