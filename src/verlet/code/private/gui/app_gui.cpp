#include "app_gui.hpp"

#include <thread>

#include "coloring/spawn_color/spawn_color_strategy.hpp"
#include "coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "coloring/tick_color/tick_color_strategy.hpp"
#include "coloring/tick_color/tick_color_strategy_velocity.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/ui/imgui_helpers.hpp"
#include "tools/delete_objects_tool.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"
#include "tools/tool.hpp"
#include "verlet_app.hpp"

namespace verlet
{

void AppGUI::Render()
{
    const klgl::ScopeAnnotation annotation("Render GUI");
    if (ImGui::Begin("Verlet"))
    {
        {
            auto color = app_->GetBackgroundColor();
            if (ImGui::ColorEdit3("Background color", color.data()))
            {
                app_->SetBackgroundColor(color);
            }
        }

        Camera();
        Perf();
        Emitter();
        Tools();
        SpawnColors();
        TickColors();
        CollisionsSolver();
    }
    ImGui::End();
}

void AppGUI::Camera()
{
    // if (!ImGui::CollapsingHeader("Camera")) return;

    // auto& camera = app_->GetCamera();
    // ImGui::SliderFloat("Zoom", &camera.zoom, 0.1f, 5.f);

    // const auto& world_range = app_->GetWorldRange();
    // GuiText("Eye");
    // ImGui::SliderFloat("x", &camera.eye.x(), world_range.x.begin, world_range.x.end);
    // ImGui::SliderFloat("y", &camera.eye.y(), world_range.y.begin, world_range.y.end);
    // ImGui::Separator();

    // ImGui::SliderFloat("Pan speed", &camera.pan_speed, 0.0001f, 1.f);
    // ImGui::SliderFloat("Zoom speed", &camera.zoom_speed, 0.0001f, 1.f);

    // if (ImGui::Button("Reset"))
    // {
    //     camera.zoom = 1.f;
    //     camera.eye = {};
    // }
}

void AppGUI::Perf()
{
    if (!ImGui::CollapsingHeader("Performance")) return;
    auto to_flt_ms = [](auto duration)
    {
        return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(duration);
    };

    const auto& stats = app_->GetPerfStats();
    GuiText("Framerate: {}", app_->GetFramerate());
    GuiText("Objects count: {}", app_->solver.objects.ObjectsCount());
    GuiText("Sim update {}", to_flt_ms(stats.sim_update.total));
    GuiText("  Apply links {}", to_flt_ms(stats.sim_update.apply_links));
    GuiText("  Rebuild grid {}", to_flt_ms(stats.sim_update.rebuild_grid));
    GuiText("  Solve collisions {}", to_flt_ms(stats.sim_update.solve_collisions));
    GuiText("  Update positions {}", to_flt_ms(stats.sim_update.update_positions));
    GuiText("Render {}", to_flt_ms(stats.render.total));
    GuiText("  Set Circle Loop {}", to_flt_ms(stats.render.set_circle_loop));
}

void AppGUI::Emitter()
{
    if (!ImGui::CollapsingHeader("Emitter")) return;

    ImGui::Checkbox("Enabled", &app_->GetEmitter().enabled);
    klgl::ImGuiHelper::SliderUInt("Max objects", &app_->GetEmitter().max_objects_count, 0uz, 150'000uz);
}

void AppGUI::Tools()
{
    if (!ImGui::CollapsingHeader("Tools")) return;

    auto use_tool = [&](const ToolType tool_type)
    {
        auto& tool = app_->tool_;
        if (!tool || tool->GetToolType() != tool_type)
        {
            switch (tool_type)
            {
            case ToolType::SpawnObjects:
                tool = std::make_unique<SpawnObjectsTool>(*app_);
                break;
            case ToolType::MoveObjects:
                tool = std::make_unique<MoveObjectsTool>(*app_);
                break;
            case ToolType::DeleteObjects:
                tool = std::make_unique<DeleteObjectsTool>(*app_);
                break;
            default:
                tool = nullptr;
                break;
            }
        }

        if (tool)
        {
            tool->DrawGUI();
        }
    };

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_DrawSelectedOverline;
    if (ImGui::BeginTabBar("Tools", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Spawn"))
        {
            use_tool(ToolType::SpawnObjects);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Move"))
        {
            use_tool(ToolType::MoveObjects);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Delete"))
        {
            use_tool(ToolType::DeleteObjects);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void AppGUI::SpawnColors()
{
    auto use = [&](const cppreflection::Type* type)
    {
        if (&app_->spawn_color_strategy_->GetType() != type)
        {
            if (type == cppreflection::GetTypeInfo<SpawnColorStrategyRainbow>())
            {
                app_->spawn_color_strategy_ = std::make_unique<SpawnColorStrategyRainbow>(*app_);
            }
        }

        app_->spawn_color_strategy_->DrawGUI();
    };

    if (ImGui::CollapsingHeader("Spawn Color"))
    {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_DrawSelectedOverline;
        if (ImGui::BeginTabBar("Spawn Color", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Rainbow"))
            {
                use(cppreflection::GetTypeInfo<SpawnColorStrategyRainbow>());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}

void AppGUI::TickColors()
{
    auto use = [&](const cppreflection::Type* type)
    {
        if (!app_->tick_color_strategy_ || &app_->tick_color_strategy_->GetType() != type)
        {
            if (type == cppreflection::GetTypeInfo<TickColorStrategyVelocity>())
            {
                app_->tick_color_strategy_ = std::make_unique<TickColorStrategyVelocity>(*app_);
            }
        }

        if (app_->tick_color_strategy_)
        {
            app_->tick_color_strategy_->DrawGUI();
        }
    };

    if (ImGui::CollapsingHeader("Tick Color"))
    {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_DrawSelectedOverline;
        if (ImGui::BeginTabBar("Tick Color", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("None"))
            {
                app_->tick_color_strategy_ = nullptr;
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Velocity"))
            {
                use(cppreflection::GetTypeInfo<TickColorStrategyVelocity>());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}

void AppGUI::CollisionsSolver()
{
    if (!ImGui::CollapsingHeader("Collisions Solver")) return;
    GuiText("0 threads means collisions will be solved in the main thread");
    klgl::ImGuiHelper::SliderGetterSetter(
        "Threads Count",
        0uz,
        size_t{std::thread::hardware_concurrency()},
        std::bind_front(&VerletSolver::GetThreadsCount, &app_->solver),
        std::bind_front(&VerletSolver::SetThreadsCount, &app_->solver));
}
}  // namespace verlet
