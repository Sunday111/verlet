#pragma once

#include "klgl/name_cache/name.hpp"

namespace klgl
{

class UniformHandle
{
public:
    uint32_t index = 0;
    Name name{};
};

}  // namespace klgl
