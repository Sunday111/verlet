#pragma once

#include <limits>

#include "edt/math/matrix.hpp"
#include "edt/template/tagged_identifier.hpp"

namespace verlet
{
using namespace edt::lazy_matrix_aliases;  // NOLINT

struct ObjectIdTag;
using ObjectId = edt::TaggedIdentifier<ObjectIdTag, size_t>;
static constexpr auto kInvalidObjectId = ObjectId{};

static constexpr uint32_t kInvalidObjectIndex = std::numeric_limits<uint32_t>::max();

class VerletObject
{
public:
    VerletObject() = default;
    VerletObject(const VerletObject&) = delete;

    Vec2f position{};
    Vec2f old_position{};

    // The object a grid cell hands out after this one. A cell holds the first of a chain
    // rather than a fixed number of slots, so however many objects crowd into one they all
    // stay in it.
    uint32_t next_object_in_cell = kInvalidObjectIndex;

    Vec4<uint8_t> color{};
    bool movable : 1 {};

    [[nodiscard]] bool IsMovable() const { return movable; }

    [[nodiscard]] static constexpr float GetRadius() { return 0.5f; }

    // This property guarantees that there is at least one bit at the endof the object
    // which can be used to mark object as dead or alive
    uint8_t reserved_property_that_goes_last : 1 {};
};

}  // namespace verlet
