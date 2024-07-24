#pragma once

#include <utility>

namespace klgl
{

template <typename Fn>
auto OnScopeLeave(Fn&& fn)
{
    struct Guard
    {
        explicit Guard(Fn&& on_leave) : on_leave_(std::forward<Fn>(on_leave)) {}
        Guard(Guard&) = delete;
        Guard(Guard&& another) noexcept : on_leave_(std::move(another.on_leave_)) { another.empty_ = true; }
        ~Guard()
        {
            if (!empty_)
            {
                on_leave_();
            }
        }

        Fn on_leave_;
        bool empty_ = false;
    } guard(std::forward<Fn>(fn));

    return guard;
}

}  // namespace klgl
