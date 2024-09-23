#pragma once

#include "klgl/macro/enum_as_index.hpp"

namespace verlet
{
enum class EmitterFlag
{
    PendingKill,
    CloneRequested,
    Enabled
};
}

KLGL_ENUM_AS_INDEX_MAGIC_ENUM_NONAMESPACE(verlet::EmitterFlag);
