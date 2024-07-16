#pragma once

#ifndef __cpp_size_t_suffix
constexpr size_t operator""uz(unsigned long long value) {
    return static_cast<size_t>(value);
}
#endif
