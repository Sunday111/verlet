#pragma once

#include <EverydayTools/Concepts/Callable.hpp>
#include <cassert>
#include <vector>

#ifndef NDEBUG
#include "ankerl/unordered_dense.h"
#include "tagged_id_hash.hpp"
#endif

#include "object.hpp"
#include "ranges.hpp"

namespace verlet
{

struct FreePoolEntry
{
    ObjectId next_free_ = kInvalidObjectId;
};

struct ObjectPoolEntry
{
    static_assert(sizeof(FreePoolEntry) < sizeof(VerletObject));
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
        assert(self.valid_ones_.contains(id));
        return self.GetSlot(id).AsObject();
    }

    template <typename Self>
    [[nodiscard]] auto Identifiers(this Self& self)
    {
        return RangeIndices(self.entries_) |
               std::views::filter([&](const size_t index) -> bool { return self.entries_[index].data_.back(); }) |
               std::views::transform([&](const size_t index) { return ObjectId::FromValue(index); });
    }

    template <typename Self>
    [[nodiscard]] auto IdentifiersAndObjects(this Self& self)
    {
        return self.Identifiers() |
               std::views::transform(
                   [&](ObjectId id) -> std::tuple<ObjectId, VerletObject&> { return {id, self.Get(id)}; });
    }

    template <typename Self>
    [[nodiscard]] auto Objects(this Self& self)
    {
        return self.Identifiers() | std::views::transform([&](ObjectId id) -> decltype(auto) { return self.Get(id); });
    }

    std::tuple<ObjectId, VerletObject&> Alloc();
    void Free(ObjectId id);
    size_t ObjectsCount() const { return count_; }

private:
    template <typename Self>
    [[nodiscard]] auto& GetSlot(this Self& self, const ObjectId& object_id)
    {
        assert(object_id.GetValue() < self.entries_.size());
        return self.entries_[object_id.GetValue()];
    }

    VerletObject& ConstructObjectSlot(ObjectPoolEntry& entry)
    {
        auto& object = entry.AsObject();
        new (&object) VerletObject();
        object.reserved_property_that_goes_last = true;
        return object;
    }

    FreePoolEntry& ConstructFreeSlot(ObjectPoolEntry& entry)
    {
        auto& free_slot = entry.AsFreeSlot();
        new (&free_slot) FreePoolEntry();
        entry.data_.back() = false;
        return free_slot;
    }

    FreePoolEntry& ConvertObjectSlotToFreeSlot(ObjectPoolEntry& entry)
    {
        entry.AsObject().~VerletObject();
        return ConstructFreeSlot(entry);
    }

    VerletObject& ConvertFreeSlotToObjectSlot(ObjectPoolEntry& entry)
    {
        entry.AsFreeSlot().~FreePoolEntry();
        return ConstructObjectSlot(entry);
    }

private:
    size_t count_ = 0;
    std::vector<ObjectPoolEntry> entries_;
    ObjectId first_free_ = kInvalidObjectId;

#ifndef NDEBUG
    ankerl::unordered_dense::set<ObjectId, TaggedIdentifierHash<ObjectId>> valid_ones_;
#endif
};
}  // namespace verlet
