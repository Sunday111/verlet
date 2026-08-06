#include "object_pool.hpp"

namespace verlet
{
std::tuple<ObjectId, VerletObject&> ObjectPool::Alloc()
{
    ++count_;
    if (first_free_.IsValid())
    {
        auto id = first_free_;
        assert(valid_ones_.insert(id).second);
        auto& entry = GetSlot(id);
        first_free_ = entry.AsFreeSlot().next_free_;
        return {id, ConvertFreeSlotToObjectSlot(entry)};
    }
    else
    {
        auto id = ObjectId::FromValue(entries_.size());
        assert(valid_ones_.insert(id).second);
        auto& entry = entries_.emplace_back();
        return {id, ConstructObjectSlot(entry)};
    }
}

void ObjectPool::Free(ObjectId id)
{
    assert(valid_ones_.erase(id) == 1);
    assert(GetSlot(id).data_.back() != 0);
    auto& free_slot = ConvertObjectSlotToFreeSlot(GetSlot(id));
    free_slot.next_free_ = first_free_;
    first_free_ = id;
    --count_;
}

void ObjectPool::Clear()
{
    for (auto& entry : entries_)
    {
        if (entry.data_.back() != 0)
        {
            entry.AsObject().~VerletObject();
        }
    }

    // Freeing the slots one by one would leave them on the free list in reverse,
    // so the next objects would be allocated back to front. Emptying the pool
    // instead hands out the same identifiers a new pool would, in the same order,
    // which is what a simulation replayed from the start has to see.
    entries_.clear();
    first_free_ = kInvalidObjectId;
    count_ = 0;

#ifndef NDEBUG
    valid_ones_.clear();
#endif
}

}  // namespace verlet
