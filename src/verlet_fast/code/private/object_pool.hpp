#pragma once

#include <cassert>
#include <vector>

#include "ankerl/unordered_dense.h"
#include "object.hpp"
#include "ranges.hpp"
#include "tagged_id_hash.hpp"

namespace verlet
{

struct FreePoolEntry
{
    ObjectId next_free_ = kInvalidObjectId;
};

struct ObjectPoolEntry
{
    static constexpr size_t kSlotSize = std::max(sizeof(FreePoolEntry), sizeof(VerletObject));
    static constexpr size_t kSlotAlignment = std::max(alignof(FreePoolEntry), alignof(VerletObject));

    template <typename Self>
    VerletObject& AsObject(this Self& self)
    {
        return *reinterpret_cast<VerletObject*>(self.data_.data());  // NOLINT
    }

    template <typename Self>
    FreePoolEntry& AsFreeSlot(this Self& self)
    {
        return *reinterpret_cast<FreePoolEntry*>(self.data_.data());  // NOLINT
    }

    alignas(kSlotAlignment) std::array<uint8_t, kSlotSize> data_;
};

class ObjectPool
{
public:
    template <typename Self>
    [[nodiscard]] auto& Get(this Self& self, const ObjectId& id)
    {
        return self.GetSlot(id).AsObject();
    }

    [[nodiscard]] auto AllObjects() const
    {
        return RangeIndices(entries_) |
               std::views::filter(
                   [&](const size_t index)
                   {
                       return occupied_[index];
                   }) |
               std::views::transform(ObjectId::FromValue);
    }

    std::tuple<ObjectId, VerletObject&> Alloc();
    void Free(ObjectId id);
    size_t ObjectsCount() const
    {
        return count_;
    }

private:
    template <typename Self>
    [[nodiscard]] auto& GetSlot(this Self& self, const ObjectId& object_id)
    {
        assert(object_id.GetValue() < self.entries_.size());
        return self.entries_[object_id.GetValue()];
    }

private:
    size_t count_ = 0;
    std::vector<ObjectPoolEntry> entries_;
    std::vector<bool> occupied_;
    ObjectId first_free_ = kInvalidObjectId;
};
}  // namespace verlet
