#pragma once

#include "EverydayTools/Math/float_range.hpp"
#include "EverydayTools/Math/int_range.hpp"
#include "EverydayTools/Math/math.hpp"
#include "EverydayTools/Math/matrix.hpp"

namespace verlet
{

template <std::floating_point T>
using FloatRange = edt::FloatRange<T>;

template <std::floating_point T>
using FloatRange2D = edt::FloatRange2D<T>;

template <std::floating_point T>
using FloatRange2D = edt::FloatRange2D<T>;

using namespace edt::lazy_matrix_aliases;  // NOLINT

}  // namespace verlet
