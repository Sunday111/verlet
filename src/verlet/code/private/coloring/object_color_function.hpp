#pragma once
#include <functional>

#include "EverydayTools/Math/Matrix.hpp"

namespace verlet
{
class VerletObject;
using ObjectColorFunction = std::function<edt::Vec4<uint8_t>(const VerletObject&)>;
}  // namespace verlet
