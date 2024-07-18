#include "object_pool.hpp"

namespace verlet
{
std::tuple<ObjectId, VerletObject&> ObjectPool::Alloc()
{
    ++count_;

    if (first_free_.IsValid())
    {
        ObjectId id = first_free_;
        ObjectPoolEntry& entry = GetSlot(id);

        {
            FreePoolEntry& empty_slot = entry.AsFreeSlot();
            first_free_ = entry.AsFreeSlot().next_free_;
            empty_slot.~FreePoolEntry();
        }

        VerletObject& object = entry.AsObject();
        new (&object) VerletObject();
        occupied_[id.GetValue()] = true;
        return {id, object};
    }

    auto id = ObjectId::FromValue(entries_.size());
    VerletObject& object = entries_.emplace_back().AsObject();
    new (&object) VerletObject();
    occupied_.push_back(true);
    return {id, object};
}

void ObjectPool::Free(ObjectId id)
{
    auto& entry = GetSlot(id);
    entry.AsObject().~VerletObject();
    auto& free_slot = entry.AsFreeSlot();
    new (&free_slot) FreePoolEntry();
    free_slot.next_free_ = first_free_;
    first_free_ = id;
    occupied_[id.GetValue()] = false;
    --count_;
}
}  // namespace verlet
