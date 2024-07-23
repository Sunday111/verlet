#pragma once

#include "EverydayTools/Math/Matrix.hpp"
#include "EverydayTools/Template/TaggedIdentifier.hpp"

namespace verlet
{
using namespace edt::lazy_matrix_aliases;  // NOLINT

struct ObjectIdTag;
using ObjectId = edt::TaggedIdentifier<ObjectIdTag, size_t>;
static constexpr auto kInvalidObjectId = ObjectId{};

class VerletObject
{
public:
    VerletObject() = default;
    VerletObject(const VerletObject&) = delete;

    Vec2f position{};
    Vec2f old_position{};
    Vec3<uint8_t> color;
    bool movable : 1 {};

    bool IsMovable() const
    {
        return movable;
    }

    [[nodiscard]] static constexpr float GetRadius()
    {
        return 0.5f;
    }

    //////
    uint8_t reserved_property_that_goes_last : 1 {};
};

static_assert(sizeof(VerletObject) == 20);
}  // namespace verlet
