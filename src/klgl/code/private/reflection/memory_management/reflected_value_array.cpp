#include "klgl/reflection/memory/reflected_value_array.hpp"

namespace klgl
{

void ReflectedValueArray::MoveObjects(const cppreflection::Type& type, uint8_t* from, uint8_t* to, size_t count)
{
    auto& special_members = type.GetSpecialMembers();
    const size_t instance_size = type.GetInstanceSize();
    for (size_t i = 0; i != count; ++i)
    {
        // move existing objects to the new buffer and delete old object
        const size_t offset = instance_size * i;
        special_members.moveConstructor(to + offset, from + offset);
        special_members.destructor(from + offset);
    }
}

void ReflectedValueArray::Realloc(size_t new_capacity, size_t shift_begin, size_t shift_size)
{
    assert(new_capacity > capacity_);

    auto [new_buffer, new_first_object] = MakeNewBuffer(*type_, new_capacity);

    const bool with_shift = (shift_size && shift_begin < count_);
    if (with_shift)
    {
        assert(!with_shift || (new_capacity - count_ >= shift_size));
        MoveObjects(*type_, first_object_, new_first_object, shift_begin);
        MoveObjects(
            *type_,
            first_object_ + shift_begin * instance_size_,
            new_first_object + (shift_begin + shift_size) * instance_size_,
            count_ - shift_begin);
    }
    else
    {
        MoveObjects(*type_, first_object_, new_first_object, count_);
    }

    std::swap(buffer_, new_buffer);
    std::swap(first_object_, new_first_object);
    capacity_ = new_capacity;
}

void ReflectedValueArray::Reserve(size_t new_capacity)
{
    if (new_capacity > capacity_)
    {
        Realloc(new_capacity, 0, 0);
    }
}

void ReflectedValueArray::Resize(size_t count)
{
    auto& special_members = type_->GetSpecialMembers();

    if (count > capacity_)
    {
        Reserve(((count / 4) + 1) * 4);

        // default initialize new elements
        for (size_t i = count_; i != count; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto new_object = first_object_ + offset;
            special_members.defaultConstructor(new_object);
        }
    }
    else if (count > count_)
    {
        // call default constructor on new objects
        for (size_t i = count_; i != count; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto object = first_object_ + offset;
            special_members.defaultConstructor(object);
        }
    }
    else if (count < count_)
    {
        // call destructor on deleted objects
        for (size_t i = count; i != count_; ++i)
        {
            const size_t offset = instance_size_ * i;
            const auto object = first_object_ + offset;
            special_members.destructor(object);
        }
    }

    count_ = count;
}

void ReflectedValueArray::Erase(const size_t index)
{
    assert(index < Size());
    auto& special_members = type_->GetSpecialMembers();

    uint8_t* current = first_object_ + instance_size_ * index;
    uint8_t* last = first_object_ + instance_size_ * (count_ - 1);

    while (current != last)
    {
        auto next = current + instance_size_;
        special_members.moveAssign(current, next);
        current = next;
    }

    special_members.destructor(last);

    --count_;
}

void ReflectedValueArray::Insert(const size_t index)
{
    assert(index <= Size());
    auto& special_members = type_->GetSpecialMembers();
    if (count_ == capacity_)
    {
        // Have to reallocate anyway so let Realloc handle shift for us
        Realloc(count_ + 1, index, 1);

        // Still have to init new object
        uint8_t* p = first_object_ + instance_size_ * index;
        special_members.defaultConstructor(p);

        ++count_;
    }
    else
    {
        // push back lucky case - just init the new element at the end
        if (index == count_)
        {
            uint8_t* p = first_object_ + instance_size_ * count_;
            special_members.defaultConstructor(p);
            ++count_;
            return;
        }

        uint8_t* current = first_object_ + instance_size_ * count_;

        // initialize the last element using the move constructor
        {
            auto prev = current - instance_size_;
            special_members.moveConstructor(current, prev);
            current = prev;
        }

        ++count_;

        uint8_t* inserted = first_object_ + instance_size_ * index;

        while (current != inserted)
        {
            auto prev = current - instance_size_;
            special_members.moveAssign(current, prev);
            current = prev;
        }
    }
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

    return {BufferPtr(ptr), ptr + offset};
}

}  // namespace klgl
