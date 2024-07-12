#pragma once

#include <fmt/core.h>

#include "cpptrace/cpptrace.hpp"

namespace klgl
{

class ErrorHandling
{
public:
    template <typename... Args>
    static void Ensure(const bool condition, fmt::format_string<Args...> format, Args&&... args)
    {
        if (!condition)
        {
            throw cpptrace::runtime_error(fmt::format(format, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    static void ThrowWithMessage(fmt::format_string<Args...> format, Args&&... args)
    {
        throw cpptrace::runtime_error(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename F>
    static void InvokeAndCatchAll(F&& f)
    {
        try
        {
            f();
        }
        catch (const cpptrace::exception& exception)
        {
            fmt::println("Unhandled exception: {}", exception.message());
            exception.trace().print();
        }
        catch (const std::exception& exception)
        {
            fmt::println("Unhandled exception: {}", exception.what());
        }
        catch (...)
        {
            fmt::println("Unhandled exception of unknown type");
        }
    }
};
}  // namespace klgl
