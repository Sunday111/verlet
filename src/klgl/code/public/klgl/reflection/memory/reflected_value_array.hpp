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

    void Reserve(size_t count);
    void Resize(size_t count);
    void Erase(size_t index);
    void Insert(size_t index);

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

    static void MoveObjects(const cppreflection::Type& type, uint8_t* from, uint8_t* to, size_t count);
    void Realloc(size_t new_capacity, size_t shift_begin, size_t shift_size);

private:
    const cppreflection::Type* type_{};
    size_t count_ = 0;
    size_t capacity_ = 0;
    size_t instance_size_ = 0;

    uint8_t* first_object_ = nullptr;
    BufferPtr buffer_{};  // NOLINT
};

}  // namespace klgl
