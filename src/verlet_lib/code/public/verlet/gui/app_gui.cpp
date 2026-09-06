#include "app_gui.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <thread>
#include <vector>

#include "klvk/platform/file_dialog.hpp"
#include "klvk/ui/imgui_helpers.hpp"
#include "klvk/vulkan/texture.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "verlet/coloring/tick_color/tick_color_strategy.hpp"
#include "verlet/coloring/tick_color/tick_color_strategy_velocity.hpp"
#include "verlet/emitters/emitter.hpp"
#include "verlet/emitters/flat_emitter.hpp"
#include "verlet/emitters/radial_emitter.hpp"
#include "verlet/tools/delete_objects_tool.hpp"
#include "verlet/tools/move_objects_tool.hpp"
#include "verlet/tools/spawn_objects_tool.hpp"
#include "verlet/tools/spawn_random_objects_tool.hpp"
#include "verlet/tools/tool.hpp"
#include "verlet/verlet_app.hpp"
#include "window_size_limits.hpp"

namespace verlet
{
namespace
{
constexpr float kSidebarMinimumWidth = 160.f;
constexpr float kInspectorGap = 8.f;

void FullWidthItem()
{
    ImGui::SetNextItemWidth(-FLT_MIN);
}

float SidebarWidth()
{
    const auto& style = ImGui::GetStyle();
    return std::max(kSidebarMinimumWidth, ImGui::CalcTextSize("  Performance").x + 2 * style.WindowPadding.x);
}
}  // namespace

void AppGUI::Render()
{
    const auto& io = ImGui::GetIO();
    if (app_->tool_ && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !io.WantTextInput &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
    {
        app_->tool_.reset();
    }

    Sidebar();
    if (inspector_open_) InspectorWindow();
    if (particle_texture_open_)
    {
        const klvk::Texture& texture = app_->GetParticleTexture();
        particle_texture_viewer_.Draw(
            app_->GetDeviceContext(),
            texture.GetView(),
            texture.GetSize(),
            "Circle mask",
            &particle_texture_open_);
    }
}

void AppGUI::Sidebar()
{
    const auto* viewport = ImGui::GetMainViewport();
    const float sidebar_width = SidebarWidth();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints({sidebar_width, 0.f}, {sidebar_width, FLT_MAX});

    constexpr auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Verlet", nullptr, flags))
    {
        if (ImGui::Button(app_->IsPaused() ? "Play" : "Pause", {74.f, 0.f}))
        {
            app_->SetPaused(!app_->IsPaused());
        }
        ImGui::SameLine();
        if (ImGui::Button("Step", {-FLT_MIN, 0.f})) app_->RequestStep();

        GuiText("Objects: {}", app_->solver.objects.ObjectsCount());
        GuiText("Budget: {}", app_->max_objects_count_);
        GuiText("FPS: {:.0f}", app_->GetFramerate());
        ImGui::Separator();

        GuiText("Tool: {}", ActiveToolName());
        ImGui::Separator();

        constexpr std::array navigation{
            std::pair{"Simulation", Inspector::Simulation},
            std::pair{"Emitters", Inspector::Emitters},
            std::pair{"Tools", Inspector::Tools},
            std::pair{"Appearance", Inspector::Appearance},
            std::pair{"Diagnostics", Inspector::Diagnostics},
            std::pair{"Performance", Inspector::Performance},
        };
        for (const auto& [name, inspector] : navigation)
        {
            const bool selected = inspector_open_ && active_inspector_ == inspector;
            const auto label = FormatTemp("{} {}###navigation {}", selected ? ">" : " ", name, name);
            if (ImGui::Selectable(label.data(), selected))
            {
                if (selected)
                {
                    inspector_open_ = false;
                }
                else
                {
                    active_inspector_ = inspector;
                    inspector_open_ = true;
                }
            }
        }
        DestructiveActionPopups();
    }
    ImGui::End();
}

void AppGUI::InspectorWindow()
{
    const auto* viewport = ImGui::GetMainViewport();
    const float sidebar_width = SidebarWidth();
    const ImVec2 default_position{viewport->WorkPos.x + sidebar_width + kInspectorGap, viewport->WorkPos.y};
    const float available_width = std::max(1.f, viewport->WorkSize.x - sidebar_width - kInspectorGap);
    const float minimum_width = std::min(320.f, available_width);
    const float minimum_height = std::min(180.f, viewport->WorkSize.y);
    const float default_width = std::min(380.f, available_width);
    const float default_height = std::min(620.f, viewport->WorkSize.y);
    ImGui::SetNextWindowPos(default_position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({default_width, default_height}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({minimum_width, minimum_height}, {available_width, viewport->WorkSize.y});

    const auto title = FormatTemp("{}###Verlet inspector", InspectorName());
    bool open = true;
    if (ImGui::Begin(title.data(), &open, ImGuiWindowFlags_NoCollapse))
    {
        const auto position = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        const ImVec2 work_end{viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y};
        const ImVec2 recovered{
            std::clamp(position.x, viewport->WorkPos.x, std::max(viewport->WorkPos.x, work_end.x - size.x)),
            std::clamp(position.y, viewport->WorkPos.y, std::max(viewport->WorkPos.y, work_end.y - size.y)),
        };
        const bool outside = position.x < viewport->WorkPos.x || position.y < viewport->WorkPos.y ||
                             position.x + size.x > work_end.x || position.y + size.y > work_end.y;
        if (outside) ImGui::SetWindowPos(recovered);

        switch (active_inspector_)
        {
        case Inspector::Simulation:
            Simulation();
            break;
        case Inspector::Emitters:
            Emitters();
            break;
        case Inspector::Tools:
            Tools();
            break;
        case Inspector::Appearance:
            Appearance();
            break;
        case Inspector::Diagnostics:
            Diagnostics();
            break;
        case Inspector::Performance:
            Performance();
            break;
        }
    }
    ImGui::End();
    if (!open) inspector_open_ = false;
}

void AppGUI::Simulation()
{
    bool by_saturation = app_->max_objects_saturation_.has_value();
    if (ImGui::Checkbox("Limit by saturation", &by_saturation))
    {
        if (by_saturation)
        {
            const auto capacity = app_->ObjectsCapacity();
            const float share =
                capacity == 0 ? 0.f : static_cast<float>(app_->max_objects_count_) / static_cast<float>(capacity);
            app_->max_objects_saturation_ = std::clamp(share, 0.f, 1.f);
        }
        else
        {
            app_->max_objects_saturation_.reset();
        }
    }

    if (app_->max_objects_saturation_)
    {
        float percent = *app_->max_objects_saturation_ * 100.f;
        GuiText("Saturation (%)");
        FullWidthItem();
        if (klvk::ImGuiHelper::FiniteSliderFloat(
                "##saturation",
                percent,
                0.f,
                100.f,
                "%.0f%%",
                ImGuiSliderFlags_AlwaysClamp))
        {
            *app_->max_objects_saturation_ = percent / 100.f;
        }
    }
    else
    {
        uint64_t budget = app_->max_objects_count_;
        const uint64_t minimum = 0;
        const uint64_t maximum = std::max<uint64_t>(app_->ObjectsCapacity(), budget);
        GuiText("Object budget");
        FullWidthItem();
        if (ImGui::SliderScalar(
                "##object budget",
                ImGuiDataType_U64,
                &budget,
                &minimum,
                &maximum,
                "%llu",
                ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
        {
            app_->max_objects_count_ = static_cast<size_t>(budget);
        }
    }
    GuiText("World capacity: {}", app_->ObjectsCapacity());

    const auto actual_window_size = app_->GetWindow().GetSize();
    if (!window_size_initialized_ || !ImGui::IsAnyItemActive())
    {
        window_size_ = actual_window_size.Cast<int>();
        window_size_initialized_ = true;
    }
    GuiText("Window size (px)");
    FullWidthItem();
    ImGui::DragInt2(
        "##window size",
        window_size_.data(),
        1.f,
        WindowSizeLimits::kMinimumExtent,
        WindowSizeLimits::kMaximumExtent,
        "%d",
        ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        app_->GetWindow().SetSize(static_cast<size_t>(window_size_.x()), static_cast<size_t>(window_size_.y()));
    }

    const int maximum_threads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    if (!thread_count_initialized_ || !ImGui::IsAnyItemActive())
    {
        thread_count_ = static_cast<int>(app_->solver.GetThreadsCount());
        thread_count_initialized_ = true;
    }
    GuiText("Collision threads");
    FullWidthItem();
    ImGui::InputInt("##collision threads", &thread_count_);
    thread_count_ = std::clamp(thread_count_, 1, maximum_threads);
    if (ImGui::IsItemDeactivatedAfterEdit()) app_->solver.SetThreadsCount(static_cast<size_t>(thread_count_));
    GuiText("Available threads: {}", maximum_threads);

    ImGui::SeparatorText("Files");
    static constexpr std::array preset_filters{klvk::FileDialog::Filter{.name = "Preset", .extensions = "json"}};
    static constexpr std::array positions_filters{
        klvk::FileDialog::Filter{.name = "Positions dump", .extensions = "txt"}};

    if (ImGui::Button("Save preset", {-FLT_MIN, 0.f}))
    {
        const auto suggested = app_->GetExecutableDir() / kDefaultPresetFileName;
        if (auto path = app_->SaveFileDialog("Save preset", preset_filters, suggested)) app_->SaveAppState(*path);
    }
    if (ImGui::Button("Load preset", {-FLT_MIN, 0.f}))
    {
        const auto suggested = app_->GetExecutableDir() / kDefaultPresetFileName;
        if (auto path = app_->OpenFileDialog("Load preset", preset_filters, suggested)) app_->LoadAppState(*path);
    }
    if (ImGui::Button("Save positions", {-FLT_MIN, 0.f}))
    {
        const auto suggested = app_->GetExecutableDir() / kDefaultPositionsDumpFileName;
        if (auto path = app_->SaveFileDialog("Save positions", positions_filters, suggested))
        {
            positions_save_error_.clear();
            try
            {
                app_->SavePositions(*path);
            }
            catch (const std::exception& error)
            {
                positions_save_error_ = error.what();
            }
        }
    }
    if (!positions_save_error_.empty()) GuiText("Could not save positions: {}", positions_save_error_);

    ImGui::SeparatorText("Objects");
    if (ImGui::Button("Delete all objects")) delete_objects_popup_requested_ = true;
}

void AppGUI::Emitters()
{
    const auto emitter_count = static_cast<size_t>(std::ranges::distance(app_->GetEmitters()));
    if (ImGui::Button("+ Radial"))
    {
        app_->AddEmitter(std::make_unique<RadialEmitter>());
        selected_emitter_ = emitter_count;
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Flat"))
    {
        app_->AddEmitter(std::make_unique<FlatEmitter>());
        selected_emitter_ = emitter_count;
    }
    if (ImGui::Button("Enable all")) app_->EnableAllEmitters();
    ImGui::SameLine();
    if (ImGui::Button("Disable all")) app_->DisableAllEmitters();

    std::vector<Emitter*> emitters;
    for (auto& emitter : app_->GetEmitters()) emitters.push_back(&emitter);
    if (emitters.empty())
    {
        ImGui::Spacing();
        GuiText("No emitters yet.");
        GuiText("Add a radial or flat emitter to begin.");
        return;
    }

    selected_emitter_ = std::min(selected_emitter_, emitters.size() - 1);
    ImGui::SeparatorText("Emitter list");
    for (size_t index = 0; index < emitters.size(); ++index)
    {
        auto& emitter = *emitters[index];
        ImGui::PushID(static_cast<int>(index));
        ImGui::Checkbox("##enabled", &emitter.enabled);
        ImGui::SameLine();
        const auto kind = emitter.GetType() == EmitterType::Radial ? "Radial" : "Flat";
        if (ImGui::Selectable(FormatTemp("{} {}", index + 1, kind).data(), selected_emitter_ == index))
        {
            selected_emitter_ = index;
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Selected emitter");
    auto& selected = *emitters[selected_emitter_];
    if (ImGui::Button("Clone"))
    {
        selected.clone_requested = true;
        selected_emitter_ = emitters.size();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) delete_emitter_popup_requested_ = true;
    selected.GUI();
}

void AppGUI::Tools()
{
    const auto selected = [this](ToolType type)
    {
        return app_->tool_ && app_->tool_->GetToolType() == type;
    };
    if (ImGui::RadioButton("None", !app_->tool_)) app_->tool_.reset();
    ImGui::SameLine();
    if (ImGui::RadioButton("Spawn", selected(ToolType::SpawnObjects))) SelectTool(ToolType::SpawnObjects);
    ImGui::SameLine();
    if (ImGui::RadioButton("Move", selected(ToolType::MoveObjects))) SelectTool(ToolType::MoveObjects);
    if (ImGui::RadioButton("Delete", selected(ToolType::DeleteObjects))) SelectTool(ToolType::DeleteObjects);
    ImGui::SameLine();
    if (ImGui::RadioButton("Random", selected(ToolType::SpawnRandomObjects))) SelectTool(ToolType::SpawnRandomObjects);

    if (!app_->tool_)
    {
        GuiText("Choose a tool to see its options.");
        return;
    }

    ImGui::SeparatorText("Tool options");
    app_->tool_->DrawGUI();
    if (auto* random = dynamic_cast<SpawnRandomObjectsTool*>(app_->tool_.get()))
    {
        if (ImGui::Button("Spawn")) (void)random->Spawn();
        ImGui::SameLine();
        if (ImGui::Button("Replace all")) replace_objects_popup_requested_ = true;
    }
}

void AppGUI::Appearance()
{
    auto color = app_->GetBackgroundColor();
    GuiText("Background");
    FullWidthItem();
    if (ImGui::ColorEdit3("##background", color.data())) app_->SetBackgroundColor(color);

    ImGui::SeparatorText("Spawn colors");
    app_->spawn_color_strategy_->DrawGUI();

    ImGui::SeparatorText("Motion colors");
    const bool none = !app_->tick_color_strategy_;
    if (ImGui::RadioButton("None", none)) app_->tick_color_strategy_.reset();
    ImGui::SameLine();
    const bool velocity = app_->tick_color_strategy_ &&
                          &app_->tick_color_strategy_->GetType() == refl::GetTypeInfo<TickColorStrategyVelocity>();
    if (ImGui::RadioButton("Velocity", velocity) && !velocity)
    {
        app_->tick_color_strategy_ = std::make_unique<TickColorStrategyVelocity>(*app_);
    }
    if (app_->tick_color_strategy_) app_->tick_color_strategy_->DrawGUI();
}

void AppGUI::Diagnostics()
{
    ImGui::Checkbox("Particle texture", &particle_texture_open_);
    ImGui::SeparatorText("World overlays");
    auto& options = app_->GetDiagnosticRenderer().options;
    ImGui::Checkbox("Enabled", &options.enabled);
    ImGui::BeginDisabled(!options.enabled);
    ImGui::Checkbox("Emitter previews", &options.show_emitters);
    ImGui::Checkbox("Links", &options.show_links);
    ImGui::EndDisabled();
    GuiText("Emitter previews include disabled emitters.");
}

void AppGUI::Performance()
{
    auto milliseconds = [](auto duration)
    {
        return std::chrono::duration<float, std::milli>{duration}.count();
    };
    const auto& stats = app_->GetPerfStats();
    GuiText("FPS: {:.1f}", app_->GetFramerate());
    GuiText("Objects: {}", app_->solver.objects.ObjectsCount());

    edt::Vec2f max_delta{};
    float max_length_squared = -1.f;
    for (const auto& object : app_->solver.objects.Objects())
    {
        const auto delta = object.position - object.old_position;
        const auto length_squared = delta.SquaredLength();
        if (length_squared <= max_length_squared) continue;
        max_length_squared = length_squared;
        max_delta = delta;
    }
    GuiText("Max movement delta: ({:.3f}, {:.3f})", max_delta.x(), max_delta.y());

    ImGui::SeparatorText("Timing");
    GuiText("Simulation: {:.3f} ms", milliseconds(stats.sim_update.total));
    GuiText("  Apply links: {:.3f} ms", milliseconds(stats.sim_update.apply_links));
    GuiText("  Rebuild grid: {:.3f} ms", milliseconds(stats.sim_update.rebuild_grid));
    GuiText("  Solve collisions: {:.3f} ms", milliseconds(stats.sim_update.solve_collisions));
    GuiText("  Update positions: {:.3f} ms", milliseconds(stats.sim_update.update_positions));
    GuiText("Render: {:.3f} ms", milliseconds(stats.render.total));
    GuiText("  Set circle loop: {:.3f} ms", milliseconds(stats.render.set_circle_loop));
}

void AppGUI::DestructiveActionPopups()
{
    if (delete_emitter_popup_requested_)
    {
        ImGui::OpenPopup("Delete emitter?");
        delete_emitter_popup_requested_ = false;
    }
    if (delete_objects_popup_requested_)
    {
        ImGui::OpenPopup("Delete all objects?");
        delete_objects_popup_requested_ = false;
    }
    if (replace_objects_popup_requested_)
    {
        ImGui::OpenPopup("Replace all objects?");
        replace_objects_popup_requested_ = false;
    }

    if (ImGui::BeginPopupModal("Delete emitter?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GuiText("Delete emitter {}?", selected_emitter_ + 1);
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Delete emitter"))
        {
            std::vector<Emitter*> emitters;
            for (auto& emitter : app_->GetEmitters()) emitters.push_back(&emitter);
            if (selected_emitter_ < emitters.size()) emitters[selected_emitter_]->pending_kill = true;
            if (emitters.size() > 1) selected_emitter_ = std::min(selected_emitter_, emitters.size() - 2);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Delete all objects?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GuiText("Delete all {} objects?", app_->solver.objects.ObjectsCount());
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(FormatTemp("Delete {} objects", app_->solver.objects.ObjectsCount()).data()))
        {
            app_->solver.DeleteAll();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Replace all objects?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GuiText("Replace all {} existing objects?", app_->solver.objects.ObjectsCount());
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(FormatTemp("Replace {} objects", app_->solver.objects.ObjectsCount()).data()))
        {
            if (auto* random = dynamic_cast<SpawnRandomObjectsTool*>(app_->tool_.get())) (void)random->ReplaceAll();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void AppGUI::SelectTool(ToolType type)
{
    if (app_->tool_ && app_->tool_->GetToolType() == type) return;
    switch (type)
    {
    case ToolType::SpawnObjects:
        app_->tool_ = std::make_unique<SpawnObjectsTool>(*app_);
        break;
    case ToolType::MoveObjects:
        app_->tool_ = std::make_unique<MoveObjectsTool>(*app_);
        break;
    case ToolType::DeleteObjects:
        app_->tool_ = std::make_unique<DeleteObjectsTool>(*app_);
        break;
    case ToolType::SpawnRandomObjects:
        app_->tool_ = std::make_unique<SpawnRandomObjectsTool>(*app_);
        break;
    }
}

std::string_view AppGUI::ActiveToolName() const
{
    if (!app_->tool_) return "None";
    switch (app_->tool_->GetToolType())
    {
    case ToolType::SpawnObjects:
        return "Spawn";
    case ToolType::MoveObjects:
        return "Move";
    case ToolType::DeleteObjects:
        return "Delete";
    case ToolType::SpawnRandomObjects:
        return "Random";
    }
    return "None";
}

std::string_view AppGUI::InspectorName() const
{
    switch (active_inspector_)
    {
    case Inspector::Simulation:
        return "Simulation";
    case Inspector::Emitters:
        return "Emitters";
    case Inspector::Tools:
        return "Tools";
    case Inspector::Appearance:
        return "Appearance";
    case Inspector::Diagnostics:
        return "Diagnostics";
    case Inspector::Performance:
        return "Performance";
    }
    return "Inspector";
}
}  // namespace verlet
