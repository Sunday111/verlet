#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "EverydayTools/Math/Math.hpp"

class PositionLookup
{
public:
    static constexpr uint16_t kInvalidIndex = std::numeric_limits<uint16_t>::max();

    struct RegionElement
    {
        uint16_t next = kInvalidIndex;
        uint16_t prev = kInvalidIndex;
    };

    struct Region
    {
        // Index of the first RegionElement in this region
        uint16_t first = kInvalidIndex;

        edt::FloatRange2D<float> range;
    };

    PositionLookup(const edt::FloatRange2D<float>& world_range, const uint8_t num_side_regions)
        : world_range_(world_range),
          region_size_{world_range.Extent() / num_side_regions},
          num_side_regions_(num_side_regions),
          regions_(edt::Math::Sqr(num_side_regions))
    {
        // Initialize regions ranges
        for (uint8_t xi = 0; xi != GetDimSize(); ++xi)
        {
            for (uint8_t yi = 0; yi != GetDimSize(); ++yi)
            {
                auto& region = regions_[yi * GetDimSize() + xi];
                region.range = {
                    .x =
                        {
                            .begin = world_range.x.begin + region_size_.x() * static_cast<float>(xi),
                            .end = world_range.x.begin + region_size_.x() * static_cast<float>(xi + 1),
                        },
                    .y =
                        {
                            .begin = world_range.y.begin + region_size_.y() * static_cast<float>(yi),
                            .end = world_range.y.begin + region_size_.y() * static_cast<float>(yi + 1),
                        },
                };
            }
        }
    }

    // Returns true if object changed region
    bool OnObjectMoved(const VerletObjects& objects, const size_t object_idx)
    {
        const auto object_index = static_cast<uint16_t>(object_idx);
        const Vec2f& position = objects.position[object_index];
        const auto region_index = PositionToRegionIndex(position);

        if (object_index < object_to_ll_.size())
        {
            if (ShouldKeepSameRegion(position, objects.radius[object_index], object_index, region_index))
            {
                return false;
            }

            // This object existed before - have to check if it has to be evicted after the movement
            if (object_to_region_index_[region_index] != region_index)
            {
                EvictObjectFromRegion(object_index, region_index);
                AddObjectToRegion(object_index, region_index);
            }
        }
        else
        {
            // This is a new object. Make a room for it
            object_to_ll_.resize(object_index + 1);
            object_to_region_index_.resize(object_index + 1);

            // It was not registered in any region yet so no need to evict from linked list
            AddObjectToRegion(object_index, region_index);
        }

        return true;
    }

    // Returns
    bool ShouldKeepSameRegion(
        [[maybe_unused]] const Vec2f& position,
        [[maybe_unused]] const float radius,
        const uint16_t object_index,
        const uint16_t best_region_index) const
    {
        // If best region index doesn't match the current one
        if (const uint8_t current_region = object_to_region_index_[object_index]; current_region != best_region_index)
        {
            // TODO: maybe try to extend the presence of object in region by making it move to another one only when it
            // is out by some percent of its radius?

            // FloatRange2D<float> safe_range = regions_[current_region].range;
            // safe_range.Enlarge(radius / 2);

            // return safe_range.Contains({position.x(), position.y()});

            return false;
        }

        return true;
    }

    void EvictObjectFromRegion(const uint16_t object_index, const uint8_t region_index)
    {
        // If this object happens to be the first element of linked list
        auto& ll = object_to_ll_[object_index];
        if (ll.prev == kInvalidIndex)
        {
            regions_[region_index].first = ll.next;
        }
        else
        {
            object_to_ll_[ll.prev].next = ll.next;
        }

        if (ll.next != kInvalidIndex)
        {
            object_to_ll_[ll.next].prev = ll.prev;
        }
    }

    void AddObjectToRegion(const uint16_t object_index, const uint8_t region_index)
    {
        auto& region = regions_[region_index];
        object_to_ll_[object_index].next = region.first;
        object_to_ll_[object_index].prev = kInvalidIndex;
        region.first = object_index;
        object_to_region_index_[object_index] = region_index;
    }

    [[nodiscard]] uint8_t PositionToRegionIndex(const Vec2f& p) const
    {
        auto xi = ToDimIndex(p.x(), world_range_.x.begin, region_size_.x());
        auto yi = ToDimIndex(p.y(), world_range_.y.begin, region_size_.y());
        return static_cast<uint8_t>(yi * GetDimSize() + xi);
    }

    uint16_t GetNextInSameRegion(const size_t object_index) const { return object_to_ll_[object_index].next; }

    const Region& GetRegion(const Vec2f& p) const { return regions_[PositionToRegionIndex(p)]; }

    [[nodiscard]] std::span<const Region> Regions() const { return regions_; }

    [[nodiscard]] uint8_t GetDimSize() const { return num_side_regions_; }

private:
    [[nodiscard]] uint8_t ToDimIndex(const float coord, const float coord_begin, const float region_extent) const
    {
        auto idx = static_cast<uint8_t>(std::max((coord - coord_begin) / region_extent, 0.f));
        idx = std::min<uint8_t>(idx, GetDimSize() - 1);
        return idx;
    }

private:
    FloatRange2D<float> world_range_;
    Vec2f region_size_;
    uint8_t num_side_regions_ = 5;

    std::vector<Region> regions_;

    // index in this vector matches index of the object
    // Value is linked list element for that object
    // So it can be used to find all objects in the same region
    std::vector<RegionElement> object_to_ll_;

    std::vector<uint8_t> object_to_region_index_;
};
