#pragma once

#include <string_view>

#include "fmt/core.h"
#include "imgui.h"

namespace verlet
{
class VerletApp;

// Having GUI logic in a separate class allows to keep app class cleaner.
class AppGUI
{
public:
    explicit AppGUI(VerletApp& app) : app_{&app} {}

    void Render();
    void Camera();
    void Perf();
    void Emitters();
    void Tools();
    void SpawnColors();
    void TickColors();
    void CollisionsSolver();

    template <typename... Args>
    void GuiText(std::string_view text)
    {
        auto a = text.data();
        auto b = a + text.size();  // NOLINT
        ImGui::TextUnformatted(a, b);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    void GuiText(const fmt::format_string<Args...>& format, Args&&... args)
    {
        GuiText(FormatTemp(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    std::string_view FormatTemp(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        temp_string_for_formatting_.clear();
        fmt::format_to(std::back_inserter(temp_string_for_formatting_), fmt, std::forward<Args>(args)...);
        return temp_string_for_formatting_;
    }

private:
    VerletApp* app_;
    std::string temp_string_for_formatting_{};
};
}  // namespace verlet
