#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

#include "edt/math/matrix.hpp"
#include "fmt/core.h"
#include "imgui.h"
#include "klvk/ui/imgui_texture_viewer.hpp"

namespace verlet
{
class VerletApp;
enum class ToolType : std::uint8_t;

class AppGUI
{
public:
    explicit AppGUI(VerletApp& app) : app_{&app} {}

    static constexpr std::string_view kDefaultPresetFileName = "VerletAppPreset.json";
    static constexpr std::string_view kDefaultPositionsDumpFileName = "VerletPositionsDump.txt";

    void Render();

private:
    void GuiText(std::string_view text) { ImGui::TextUnformatted(text.begin(), text.end()); }

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    void GuiText(const fmt::format_string<Args...>& format, Args&&... args)
    {
        GuiText(FormatTemp(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    std::string_view FormatTemp(const fmt::format_string<Args...>& format, Args&&... args)
    {
        temp_string_for_formatting_.clear();
        fmt::format_to(std::back_inserter(temp_string_for_formatting_), format, std::forward<Args>(args)...);
        return temp_string_for_formatting_;
    }

    enum class Inspector : std::uint8_t
    {
        Simulation,
        Emitters,
        Tools,
        Appearance,
        Diagnostics,
        Performance
    };

    void Sidebar();
    void InspectorWindow();
    void Simulation();
    void Emitters();
    void Tools();
    void Appearance();
    void Diagnostics();
    void Performance();
    void DestructiveActionPopups();
    void SelectTool(ToolType tool_type);

    [[nodiscard]] std::string_view ActiveToolName() const;
    [[nodiscard]] std::string_view InspectorName() const;

    VerletApp* app_;
    std::string temp_string_for_formatting_{};
    Inspector active_inspector_ = Inspector::Simulation;
    bool inspector_open_ = true;
    size_t selected_emitter_ = 0;
    bool delete_emitter_popup_requested_ = false;
    bool delete_objects_popup_requested_ = false;
    bool replace_objects_popup_requested_ = false;
    bool window_size_initialized_ = false;
    bool thread_count_initialized_ = false;
    edt::Vec2i window_size_{};
    int thread_count_ = 1;
    klvk::ImGuiTextureViewer imgui_font_atlas_viewer_{"Dear ImGui font atlas"};
    bool imgui_font_atlas_open_ = false;
};
}  // namespace verlet
