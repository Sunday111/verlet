#include "app_gui.hpp"

#include <thread>

#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/ui/imgui_helpers.hpp"
#include "klgl/ui/simple_type_widget.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "verlet/coloring/tick_color/tick_color_strategy.hpp"
#include "verlet/coloring/tick_color/tick_color_strategy_velocity.hpp"
#include "verlet/emitters/emitter.hpp"
#include "verlet/emitters/radial_emitter.hpp"
#include "verlet/tools/delete_objects_tool.hpp"
#include "verlet/tools/move_objects_tool.hpp"
#include "verlet/tools/spawn_objects_tool.hpp"
#include "verlet/tools/tool.hpp"
#include "verlet/verlet_app.hpp"

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

        klgl::ImGuiHelper::SliderUInt("Max objects", &app_->max_objects_count_, size_t{0}, size_t{150'000});

        if (auto window_size_f = app_->GetWindow().GetSize2f(); klgl::SimpleTypeWidget("Window size:", window_size_f))
        {
            auto window_size = edt::Math::Clamp(window_size_f, Vec2f{100, 100}, Vec2f{5000, 5000}).Cast<size_t>();
            app_->GetWindow().SetSize(window_size.x(), window_size.y());
        }

        if (ImGui::Button("Save Preset"))
        {
            app_->SaveAppState(app_->GetExecutableDir() / kDefaultPresetFileName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Preset"))
        {
            app_->LoadAppState(app_->GetExecutableDir() / kDefaultPresetFileName);
        }

        if (ImGui::Button("Save positions"))
        {
            app_->SavePositions(app_->GetExecutableDir() / kDefaultPositionsDumpFileName);
        }

        Camera();
        Perf();
        Emitters();
        Tools();
        SpawnColors();
        TickColors();
        CollisionsSolver();
        Stats();
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

void AppGUI::Emitters()
{
    if (ImGui::CollapsingHeader("Emitters"))
    {
        for (auto& emitter : app_->GetEmitters())
        {
            emitter.GUI();
        }

        if (ImGui::Button("New Radial"))
        {
            app_->AddEmitter(std::make_unique<RadialEmitter>());
        }

        if (ImGui::Button("Enable All"))
        {
            app_->EnableAllEmitters();
        }

        ImGui::SameLine();

        if (ImGui::Button("Disable All"))
        {
            app_->DisableAllEmitters();
        }
    }
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
    klgl::ImGuiHelper::SliderGetterSetter(
        "Threads Count",
        size_t{1},
        size_t{std::thread::hardware_concurrency()},
        std::bind_front(&VerletSolver::GetThreadsCount, &app_->solver),
        std::bind_front(&VerletSolver::SetThreadsCount, &app_->solver));
}

void AppGUI::Stats()
{
    if (!ImGui::CollapsingHeader("Stats")) return;

    edt::Vec2f max;
    float max_lsq = -1.f;
    for (const auto& object : app_->solver.objects.Objects())
    {
        auto delta = object.position - object.old_position;
        auto lsq = delta.SquaredLength();

        if (max_lsq < lsq)
        {
            max_lsq = lsq;
            max = delta;
        }
    }

    klgl::SimpleTypeWidget("Max Delta", max);
}
}  // namespace verlet
