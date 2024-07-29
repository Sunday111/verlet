#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "CppReflection/Detail/MakeTypeSpecialMembers.hpp"
#include "CppReflection/TypeSpecialMembers.hpp"

namespace klgl
{
class TypeErasedArray
{
private:
    using BufferPtr = std::unique_ptr<uint8_t, decltype([](uint8_t* p) { std::free(p); })>;  // NOLINT

public:
    struct TypeInfo
    {
        [[nodiscard]] constexpr bool operator==(const TypeInfo&) const = default;
        [[nodiscard]] constexpr bool operator!=(const TypeInfo&) const = default;

        cppreflection::TypeSpecialMembers special_members{};
        uint32_t alignment = 0;
        uint32_t object_size = 0;
    };

    explicit TypeErasedArray(TypeInfo type_info) : type_(type_info) {}
    TypeErasedArray(TypeErasedArray&& other) noexcept { MoveFrom(other); }
    TypeErasedArray& operator=(TypeErasedArray&& other) noexcept { return MoveFrom(other); }  // NOLINT
    TypeErasedArray(const TypeErasedArray& other) { CopyFrom(other); }
    TypeErasedArray& operator=(const TypeErasedArray& other) noexcept { return CopyFrom(other); }  // NOLINT

    void Clear(bool release_memory = false);
    TypeErasedArray& MoveFrom(TypeErasedArray& other);
    TypeErasedArray& CopyFrom(const TypeErasedArray& other);

    void Reserve(size_t new_capacity);
    void Resize(size_t count);
    void Insert(size_t index);
    void Erase(size_t index);

    [[nodiscard]] size_t Size() const { return count_; }
    [[nodiscard]] size_t Capacity() const { return capacity_; }
    [[nodiscard]] size_t CapacityBytes() const;
    [[nodiscard]] const TypeInfo& GetType() const { return type_; }

    template <typename Self>
    [[nodiscard]] auto* operator[](this Self&& self, const size_t index)
    {
        using ResultType = std::conditional_t<std::is_const_v<Self>, const void*, void*>;
        assert(index < self.Size());
        return static_cast<ResultType>(self.first_object_ + index * self.type_.object_size);
    }

    template <typename T>
    [[nodiscard]] static TypeErasedArray Create()
    {
        return TypeErasedArray(TypeErasedArray::TypeInfo{
            .special_members = cppreflection::detail::MakeTypeSpecialMembers<T>(),
            .alignment = alignof(T),
            .object_size = sizeof(T),
        });
    }

private:
    static std::tuple<BufferPtr, uint8_t*> MakeNewBuffer(const TypeInfo& type, size_t objects_count);
    static void MoveAndDestroyObjects(const TypeInfo& type, uint8_t* from, uint8_t* to, size_t count);
    static void DestroyObjects(const TypeInfo& type, uint8_t* first, size_t count);
    static void CopyObjects(const TypeInfo& type, uint8_t* from, uint8_t* to, size_t count);
    void ChangeBufferType(const TypeInfo& type);

    void Realloc(size_t new_capacity, size_t shift_begin, size_t shift_size);

private:
    TypeInfo type_;
    size_t count_ = 0;
    size_t capacity_ = 0;

    uint8_t* first_object_ = nullptr;
    BufferPtr buffer_{};  // NOLINT
};

template <typename T>
[[nodiscard]] inline auto MakeTypeErasedArray()
{
    return TypeErasedArray::Create<T>();
}

}  // namespace klgl
