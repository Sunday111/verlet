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
    assert(GetSlot(id).AsObject().reserved_property_that_goes_last);
    auto& free_slot = ConvertObjectSlotToFreeSlot(GetSlot(id));
    free_slot.next_free_ = first_free_;
    first_free_ = id;
    --count_;
}
}  // namespace verlet
