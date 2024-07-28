#include "klgl/memory/type_erased_array.hpp"

#include <cassert>

namespace klgl
{
std::tuple<TypeErasedArray::BufferPtr, uint8_t*> TypeErasedArray::MakeNewBuffer(
    const TypeInfo& type,
    size_t objects_count)
{
    size_t required_size = type.alignment + type.object_size * objects_count;
    auto ptr = reinterpret_cast<uint8_t*>(std::malloc(required_size));  // NOLINT

    size_t offset = 0;
    if (const size_t address = std::bit_cast<size_t>(ptr); address % type.alignment)
    {
        offset = type.alignment - (address % type.alignment);
    }

    return {BufferPtr(ptr), ptr + offset};
}

void TypeErasedArray::MoveObjects(const TypeInfo& type, uint8_t* from, uint8_t* to, size_t count)
{
    for (size_t i = 0; i != count; ++i)
    {
        // move existing objects to the new buffer and delete old object
        const size_t offset = type.object_size * i;
        type.special_members.moveConstructor(to + offset, from + offset);
        type.special_members.destructor(from + offset);
    }
}

void TypeErasedArray::Realloc(size_t new_capacity, size_t shift_begin, size_t shift_size)
{
    assert(new_capacity > capacity_);

    auto [new_buffer, new_first_object] = MakeNewBuffer(type_, new_capacity);

    const bool with_shift = (shift_size && shift_begin < count_);
    if (with_shift)
    {
        assert(!with_shift || (new_capacity - count_ >= shift_size));
        MoveObjects(type_, first_object_, new_first_object, shift_begin);
        MoveObjects(
            type_,
            first_object_ + shift_begin * type_.object_size,
            new_first_object + (shift_begin + shift_size) * type_.object_size,
            count_ - shift_begin);
    }
    else
    {
        MoveObjects(type_, first_object_, new_first_object, count_);
    }

    std::swap(buffer_, new_buffer);
    std::swap(first_object_, new_first_object);
    capacity_ = new_capacity;
}

void TypeErasedArray::Reserve(size_t new_capacity)
{
    if (new_capacity > capacity_)
    {
        Realloc(new_capacity, 0, 0);
    }
}
void TypeErasedArray::Resize(size_t count)
{
    if (count > capacity_)
    {
        Reserve(((count / 4) + 1) * 4);

        // default initialize new elements
        for (size_t i = count_; i != count; ++i)
        {
            const size_t offset = type_.object_size * i;
            const auto new_object = first_object_ + offset;
            type_.special_members.defaultConstructor(new_object);
        }
    }
    else if (count > count_)
    {
        // call default constructor on new objects
        for (size_t i = count_; i != count; ++i)
        {
            const size_t offset = type_.object_size * i;
            const auto object = first_object_ + offset;
            type_.special_members.defaultConstructor(object);
        }
    }
    else if (count < count_)
    {
        // call destructor on deleted objects
        for (size_t i = count; i != count_; ++i)
        {
            const size_t offset = type_.object_size * i;
            const auto object = first_object_ + offset;
            type_.special_members.destructor(object);
        }
    }

    count_ = count;
}

void TypeErasedArray::Insert(const size_t index)
{
    assert(index <= Size());
    if (count_ == capacity_)
    {
        // Have to reallocate anyway so let Realloc handle shift for us
        Realloc(count_ + 1, index, 1);

        // Still have to init new object
        uint8_t* p = first_object_ + type_.object_size * index;
        type_.special_members.defaultConstructor(p);

        ++count_;
    }
    else
    {
        // push back lucky case - just init the new element at the end
        if (index == count_)
        {
            uint8_t* p = first_object_ + type_.object_size * count_;
            type_.special_members.defaultConstructor(p);
            ++count_;
            return;
        }

        uint8_t* current = first_object_ + type_.object_size * count_;

        // initialize the last element using the move constructor
        {
            auto prev = current - type_.object_size;
            type_.special_members.moveConstructor(current, prev);
            current = prev;
        }

        ++count_;

        uint8_t* inserted = first_object_ + type_.object_size * index;

        while (current != inserted)
        {
            auto prev = current - type_.object_size;
            type_.special_members.moveAssign(current, prev);
            current = prev;
        }
    }
}

void TypeErasedArray::Erase(const size_t index)
{
    assert(index < Size());

    uint8_t* current = first_object_ + type_.object_size * index;
    uint8_t* last = first_object_ + type_.object_size * (count_ - 1);

    while (current != last)
    {
        auto next = current + type_.object_size;
        type_.special_members.moveAssign(current, next);
        current = next;
    }

    type_.special_members.destructor(last);

    --count_;
}
}  // namespace klgl
