#pragma once

#include "reflected_value_array.hpp"

namespace klgl
{
template <typename T, typename ValueArray>
    requires(std::same_as<std::remove_const_t<ValueArray>, ReflectedValueArray>)
class ReflectedValueArrayAdapter
{
public:
    static constexpr bool kConstValue = std::is_const_v<ValueArray>;
    using ValueType = std::conditional_t<kConstValue, const T, T>;
    using ValueRef = ValueType&;
    explicit ReflectedValueArrayAdapter(ValueArray& value_array) : value_array_(&value_array)
    {
        const auto type_info_t = cppreflection::GetTypeInfo<T>();
        klgl::ErrorHandling::Ensure(type_info_t != nullptr, "Could not get type info for specified type T");
        const auto type_info_arr = &value_array_->GetType();
        klgl::ErrorHandling::Ensure(
            type_info_t == type_info_arr,
            "Type info for template parameter T ({}) does not match type info contained in value array ({}).",
            type_info_t->GetName(),
            type_info_arr->GetName());
    }

    [[nodiscard]] size_t Size() const { return value_array_->Size(); }

    [[nodiscard]] ValueRef operator[](const size_t index) const
    {
        return *reinterpret_cast<ValueType*>(value_array_->operator[](index));  // NOLINT
    }

private:
    ValueArray* value_array_{};
};
}  // namespace klgl
