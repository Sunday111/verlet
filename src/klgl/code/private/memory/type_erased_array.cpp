#include "klgl/memory/type_erased_array.hpp"

#include <cassert>

namespace klgl
{

void TypeErasedArray::Clear(bool release_memory)
{
    auto begin = first_object_;
    auto end = begin + type_.object_size * count_;
    for (auto p = begin; p != end; p += type_.object_size)
    {
        type_.special_members.destructor(p);
    }

    count_ = 0;

    if (release_memory)
    {
        capacity_ = 0;
        first_object_ = nullptr;
        buffer_ = nullptr;
    }
}

TypeErasedArray& TypeErasedArray::MoveFrom(TypeErasedArray& other)
{
    if (CapacityBytes() > other.CapacityBytes() && other.Size() == 0)
    {
        // Reuse capacity if rhs object is empty
        Clear();
        ChangeBufferType(other.type_);
    }
    else
    {
        type_ = other.type_;
        other.type_ = {};

        count_ = other.count_;
        other.count_ = {};

        capacity_ = other.capacity_;
        other.capacity_ = {};

        first_object_ = other.first_object_;
        other.first_object_ = {};

        buffer_ = std::move(other.buffer_);
    }

    return *this;
}

TypeErasedArray& TypeErasedArray::CopyFrom(const TypeErasedArray& other)
{
    if (type_ == other.type_)
    {
        if (other.count_ <= capacity_)
        {
            // 1. Same type, enough space

            // copy assign
            {
                auto src = other.first_object_;
                auto dst = first_object_;
                auto dst_end = first_object_ + std::min(count_, other.count_) * type_.object_size;
                while (dst != dst_end)
                {
                    type_.special_members.copyAssign(dst, src);
                    dst += type_.object_size;
                    src += type_.object_size;
                }
            }

            if (other.count_ > count_)
            {
                // move construct
                auto src = other.first_object_ + count_ * type_.object_size;
                auto dst = first_object_ + count_ * type_.object_size;
                auto dst_end = first_object_ + other.count_ * type_.object_size;
                while (dst != dst_end)
                {
                    type_.special_members.moveConstructor(dst, src);
                    src += type_.object_size;
                    dst += type_.object_size;
                }
            }
            else
            {
                // call destructor on excessive amount
                auto p = first_object_ + other.count_ * type_.object_size;
                auto last = first_object_ + count_ * type_.object_size;
                while (p != last)
                {
                    type_.special_members.destructor(p);
                    p += type_.object_size;
                }
            }

            count_ = other.count_;
        }
        else
        {
            // 2. Same type but need more memory

            Clear();
            Realloc(other.count_, 0, 0);

            // move construct new objects here.
            auto src = other.first_object_;
            auto dst = first_object_;
            auto dst_end = first_object_ + other.count_ * type_.object_size;
            while (dst != dst_end)
            {
                type_.special_members.moveConstructor(dst, src);
                dst += type_.object_size;
                src += type_.object_size;
            }

            count_ = other.count_;
        }
    }
    else
    {
        // 3. Different types
        Clear();
        ChangeBufferType(other.type_);
        Reserve(other.count_);

        // Run this method again but this time with the same type and enough space (case 1)
        CopyFrom(other);
    }

    return *this;
}

[[nodiscard]] size_t TypeErasedArray::CapacityBytes() const
{
    if (capacity_ == 0) return 0;
    return std::bit_cast<size_t>(buffer_.get()) - std::bit_cast<size_t>(first_object_) + capacity_ * type_.object_size;
}

void TypeErasedArray::ChangeBufferType(const TypeInfo& type)
{
    assert(count_ == 0);  // This is only allowed for empty arrays

    // Try to reuse the memory used by objects of previous type
    if (capacity_ != 0)
    {
        // num bytes used for alignment plus bytes used for objects
        const size_t num_bytes = CapacityBytes();

        size_t offset = 0;
        if (const size_t address = std::bit_cast<size_t>(buffer_.get()); address % type.alignment)
        {
            offset = type.alignment - (address % type.alignment);
        }

        capacity_ = (num_bytes - std::min(offset, num_bytes)) / type.object_size;
        // might be invalid pointer but it should not be dereferenced as capacity will be zero in this case
        first_object_ = buffer_.get() + offset;
    }

    type_ = type;
}

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

void TypeErasedArray::MoveAndDestroyObjects(const TypeInfo& type, uint8_t* from, uint8_t* to, size_t count)
{
    for (size_t i = 0; i != count; ++i)
    {
        // move existing objects to the new buffer and delete old object
        const size_t offset = type.object_size * i;
        type.special_members.moveConstructor(to + offset, from + offset);
        type.special_members.destructor(from + offset);
    }
}

void TypeErasedArray::DestroyObjects(const TypeInfo& type, uint8_t* first, size_t count)
{
    auto end = first + count * type.object_size;
    for (auto p = first; p != end; p += type.object_size)
    {
        type.special_members.destructor(p);
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
        MoveAndDestroyObjects(type_, first_object_, new_first_object, shift_begin);
        MoveAndDestroyObjects(
            type_,
            first_object_ + shift_begin * type_.object_size,
            new_first_object + (shift_begin + shift_size) * type_.object_size,
            count_ - shift_begin);
    }
    else
    {
        MoveAndDestroyObjects(type_, first_object_, new_first_object, count_);
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
