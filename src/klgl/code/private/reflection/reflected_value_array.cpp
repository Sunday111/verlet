#include "klgl/reflection/reflected_value_array.hpp"

namespace klgl
{

void ReflectedValueArray::Resize(size_t count)
{
    auto& special_members = type_->GetSpecialMembers();

    if (count > capacity_)
    {
        // Need to make a new buffer, move existing objects there and delete old buffer
        const size_t new_capacity = ((count / 4) + 1) * 4;
        auto [new_buffer, new_first_object] = MakeNewBuffer(*type_, new_capacity);

        for (size_t i = 0; i != count; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto new_object = new_first_object + offset;

            if (i < count_)
            {
                // move existing objects to the new buffer and delete old object
                const auto old_object = first_object_ + offset;  // NOLINT
                special_members.moveConstructor(new_object, old_object);
                special_members.destructor(old_object);
            }
            else
            {
                // default initialize new objects
                special_members.defaultConstructor(new_object);
            }
        }

        std::swap(buffer_, new_buffer);
        std::swap(first_object_, new_first_object);
        count_ = count;
        capacity_ = new_capacity;
    }
    else if (count > count_)
    {
        // call default constructor on new objects
        for (size_t i = count_; i != count; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto object = first_object_ + offset;  // NOLINT
            special_members.defaultConstructor(object);
        }

        count_ = count;
    }
    else if (count < count_)
    {
        // call destructor on deleted objects
        for (size_t i = count; i != count_; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto object = first_object_ + offset;  // NOLINT
            special_members.destructor(object);
        }

        count_ = count;
    }
}

void ReflectedValueArray::Erase(const size_t index)
{
    assert(index < Size());
    auto& special_members = type_->GetSpecialMembers();

    uint8_t* current = first_object_;
    std::advance(current, instance_size_ * index);

    uint8_t* last = first_object_;
    std::advance(last, instance_size_ * (count_ - 1));

    while (current != last)
    {
        auto next = current;
        std::advance(next, instance_size_);

        special_members.moveAssign(current, next);
        current = next;
    }

    special_members.destructor(last);

    --count_;
}

std::tuple<ReflectedValueArray::BufferPtr, uint8_t*> ReflectedValueArray::MakeNewBuffer(
    const cppreflection::Type& type,
    size_t objects_count)
{
    size_t object_size = type.GetInstanceSize();
    size_t alignment = type.GetAlignment();
    size_t required_size = alignment + object_size * objects_count;
    auto ptr = reinterpret_cast<uint8_t*>(std::malloc(required_size));  // NOLINT

    size_t offset = 0;
    if (const size_t address = std::bit_cast<size_t>(ptr); address % alignment)
    {
        offset = alignment - (address % alignment);
    }

    return {BufferPtr(ptr), ptr + offset};  // NOLINT
}

}  // namespace klgl
