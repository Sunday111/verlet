#pragma once

// clang-format off
#ifdef __clang__
    #define warning_push _Pragma("GCC diagnostic push")
    #define warning_pop _Pragma("GCC diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define warning_push _Pragma("GCC diagnostic push")
    #define warning_pop _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
    #define warning_push _Pragma("warning(push)")
    #define warning_pop _Pragma("warning(pop)")
#endif
// clang-format on
