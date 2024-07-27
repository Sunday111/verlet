#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

#include "CppReflection/GetTypeInfo.hpp"
#include "CppReflection/Type.hpp"
#include "klgl/error_handling.hpp"

namespace klgl
{

class ReflectedValueArray
{
public:
    using BufferPtr = std::unique_ptr<uint8_t, decltype([](uint8_t* p) { std::free(p); })>;  // NOLINT

    explicit ReflectedValueArray(const cppreflection::Type& type) : type_(&type), instance_size_(type.GetInstanceSize())
    {
    }

    void Resize(size_t count);
    void Erase(size_t index);

    [[nodiscard]] size_t Size() const { return count_; }
    [[nodiscard]] const cppreflection::Type& GetType() const { return *type_; }

    template <typename Self>
    [[nodiscard]] auto* operator[](this Self&& self, const size_t index)
    {
        using ResultType = std::conditional_t<std::is_const_v<Self>, const void*, void*>;
        assert(index < self.Size());
        return static_cast<ResultType>(self.first_object_ + index * self.instance_size_);
    }

private:
    static std::tuple<BufferPtr, uint8_t*> MakeNewBuffer(const cppreflection::Type& type, size_t objects_count);

private:
    const cppreflection::Type* type_{};
    size_t count_ = 0;
    size_t capacity_ = 0;
    size_t instance_size_ = 0;

    uint8_t* first_object_ = nullptr;
    BufferPtr buffer_{};  // NOLINT
};

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
