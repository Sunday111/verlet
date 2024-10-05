#pragma once

#include <EverydayTools/Concepts/Callable.hpp>
#include <cassert>
#include <vector>

#ifndef NDEBUG
#include "klgl/template/tagged_id_hash.hpp"
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

    VerletObject& AsObject()
    {
        return *reinterpret_cast<VerletObject*>(data_.data());  // NOLINT
    }

    const VerletObject& AsObject() const
    {
        return *reinterpret_cast<const VerletObject*>(data_.data());  // NOLINT
    }

    FreePoolEntry& AsFreeSlot()
    {
        return *reinterpret_cast<FreePoolEntry*>(data_.data());  // NOLINT
    }

    alignas(kSlotAlignment) std::array<uint8_t, kSlotSize> data_;
};

class ObjectPool
{
public:
    [[nodiscard]] VerletObject& Get(const ObjectId& id)
    {
        assert(valid_ones_.contains(id));
        return GetSlot(id).AsObject();
    }

    [[nodiscard]] const VerletObject& Get(const ObjectId& id) const
    {
        assert(valid_ones_.contains(id));
        return GetSlot(id).AsObject();
    }

    [[nodiscard]] auto Identifiers() const
    {
        return RangeIndices(entries_) |
               std::views::filter([&](const size_t index) -> bool { return entries_[index].data_.back(); }) |
               std::views::transform([&](const size_t index) { return ObjectId::FromValue(index); });
    }

    [[nodiscard]] auto IdentifiersAndObjects()
    {
        return Identifiers() | std::views::transform(
                                   [&](ObjectId id) -> std::tuple<ObjectId, VerletObject&> {
                                       return {id, Get(id)};
                                   });
    }

    [[nodiscard]] auto IdentifiersAndObjects() const
    {
        return Identifiers() | std::views::transform(
                                   [&](ObjectId id) -> std::tuple<ObjectId, const VerletObject&> {
                                       return {id, Get(id)};
                                   });
    }

    [[nodiscard]] auto Objects()
    {
        return Identifiers() | std::views::transform([&](ObjectId id) -> decltype(auto) { return Get(id); });
    }

    [[nodiscard]] auto Objects() const
    {
        return Identifiers() | std::views::transform([&](ObjectId id) -> decltype(auto) { return Get(id); });
    }

    std::tuple<ObjectId, VerletObject&> Alloc();
    void Free(ObjectId id);
    size_t ObjectsCount() const { return count_; }

    void Clear();

private:
    [[nodiscard]] ObjectPoolEntry& GetSlot(const ObjectId& object_id)
    {
        assert(object_id.GetValue() < entries_.size());
        return entries_[object_id.GetValue()];
    }

    [[nodiscard]] const ObjectPoolEntry& GetSlot(const ObjectId& object_id) const
    {
        assert(object_id.GetValue() < entries_.size());
        return entries_[object_id.GetValue()];
    }

    VerletObject& ConstructObjectSlot(ObjectPoolEntry& entry)
    {
        auto& object = entry.AsObject();
        new (&object) VerletObject();
        entry.data_.back() |= 0b1000000;
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
