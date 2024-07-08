#pragma once

#include <chrono>
#include <functional>

template <typename DurationType = std::chrono::nanoseconds, typename F, typename... Args>
    requires std::invocable<F, Args...>
auto MeasureTime(F&& f, Args&&... args)
{
    using Clock = std::chrono::high_resolution_clock;
    auto get_duration = [t0 = Clock::now()]()
    {
        return std::chrono::duration_cast<DurationType>(Clock::now() - t0);
    };

    using R = std::invoke_result_t<F, Args...>;
    if constexpr (std::same_as<void, R>)
    {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        return std::tuple{get_duration()};
    }
    else
    {
        auto result = std::invoke_r<R>(std::forward<F>(f), std::forward<Args>(args)...);
        return std::tuple{get_duration(), std::move(result)};
    }
}
