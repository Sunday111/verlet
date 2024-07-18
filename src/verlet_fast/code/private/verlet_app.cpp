#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "imgui_helpers.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "ranges.hpp"
#include "tools/delete_objects_tool.hpp"
#include "tools/move_objects_tool.hpp"
#include "tools/spawn_objects_tool.hpp"

namespace verlet
{

VerletApp::VerletApp() = default;
VerletApp::~VerletApp() = default;

void VerletApp::Initialize()
{
    Super::Initialize();
    InitializeRendering();
}

void VerletApp::InitializeRendering()
{
    SetTargetFramerate({60.f});

    // klgl::OpenGl::SetClearColor(Vec4f{255, 245, 153, 255} / 255.f);
    klgl::OpenGl::SetClearColor({});
    GetWindow().SetSize(1000, 1000);
    GetWindow().SetTitle("Verlet");

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    klgl::Shader::shaders_dir_ = shaders_dir;

    shader_ = std::make_unique<klgl::Shader>("just_color.shader.json");
    shader_->Use();

    circle_painter_.Initialize();
}

void VerletApp::UpdateWorldRange()
{
    // Handle aspect ratio here: if some side of viewport becomes bigger than other -
    // we react by increasing the world coordinate range on that axis
    const auto [smaller, bigger, ratio] = [&]()
    {
        if (auto [width, height] = GetWindow().GetSize2f().Tuple(); width > height)
        {
            return std::tuple{&world_range.y, &world_range.x, width / height};
        }
        else
        {
            return std::tuple{&world_range.x, &world_range.y, height / width};
        }
    }();

    *smaller = kMinSideRange;
    *bigger = smaller->Enlarged(smaller->Extent() * (ratio - 1.f) * 0.5f);

    static constexpr float kMaxExtentChangePerTick = 0.5f;
    auto adjust_range = [](const edt::FloatRange<float>& world, edt::FloatRange<float>& sim)
    {
        if (world.begin < sim.begin)
        {
            sim.begin = world.begin;
        }
        else
        {
            sim.begin += std::min(kMaxExtentChangePerTick, world.begin - sim.begin);
        }

        if (world.end > sim.end)
        {
            sim.end = world.end;
        }
        else
        {
            sim.end -= std::min(kMaxExtentChangePerTick, sim.end - world.end);
        }
    };

    adjust_range(world_range.x, solver.sim_area_.x);
    adjust_range(world_range.y, solver.sim_area_.y);
}

void VerletApp::UpdateSimulation()
{
    if (tool_)
    {
        tool_->Tick();
    }

    // Emitter
    if (enable_emitter_ && solver.objects.ObjectsCount() <= emitter_max_objects_count_)
    {
        for (uint32_t i{40}; i--;)
        {
            const size_t idx = solver.objects.ObjectsCount();
            const float y = 50.f + 1.02f * static_cast<float>(i);
            const auto color = edt::Math::GetRainbowColors(static_cast<float>(idx) / 4000);

            {
                auto [aid, object] = solver.objects.Alloc();
                object.position = {0.6f, y};
                object.old_position = {0.4f, y};
                object.color = color;
                object.movable = true;
            }

            {
                auto [bid, object] = solver.objects.Alloc();
                object.position = {-0.6f, y};
                object.old_position = {-0.4f, y};
                object.color = color;
                object.movable = true;
            }
        }
    }

    perf_stats_.sim_update = solver.Update();
}

void VerletApp::Render()
{
    RenderWorld();
    RenderGUI();
}

void VerletApp::RenderWorld()
{
    const klgl::ScopeAnnotation annotation("Render World");

    circle_painter_.num_circles_ = 0;
    const edt::FloatRange2D<float> screen_range{.x = {-1, 1}, .y = {-1, 1}};
    const Mat3f world_to_screen = edt::Math::MakeTransform(world_range, screen_range);

    auto paint_instanced_object = [&, next_instance_index = 0uz](const VerletObject& object) mutable
    {
        const auto screen_pos = TransformPos(world_to_screen, object.position);
        const auto screen_size = TransformVector(world_to_screen, object.GetRadius() + Vec2f{});
        const auto& color = object.color;
        circle_painter_.SetCircle(next_instance_index++, screen_pos, color, screen_size);
    };

    perf_stats_.render.total = edt::MeasureTime(
        [&]
        {
            klgl::OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            perf_stats_.render.set_circle_loop = edt::MeasureTime(
                [&]
                {
                    for (const VerletObject& object : solver.objects.Objects())
                    {
                        paint_instanced_object(object);
                    }
                });

            shader_->Use();
            circle_painter_.Render();
        });
}

void VerletApp::RenderGUI()
{
    auto to_flt_ms = [](auto duration)
    {
        return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(duration);
    };

    const klgl::ScopeAnnotation annotation("Render GUI");
    if (ImGui::Begin("Settings"))
    {
        shader_->DrawDetails();
        GuiText("Framerate: {}", GetFramerate());
        GuiText("Objects count: {}", solver.objects.ObjectsCount());
        GuiText("Sim update {}", to_flt_ms(perf_stats_.sim_update.total));
        GuiText("  Apply links {}", to_flt_ms(perf_stats_.sim_update.apply_links));
        GuiText("  Rebuild grid {}", to_flt_ms(perf_stats_.sim_update.rebuild_grid));
        GuiText("  Solve collisions {}", to_flt_ms(perf_stats_.sim_update.solve_collisions));
        GuiText("  Update positions {}", to_flt_ms(perf_stats_.sim_update.update_positions));
        GuiText("Render {}", to_flt_ms(perf_stats_.render.total));
        GuiText("  Set Circle Loop {}", to_flt_ms(perf_stats_.render.set_circle_loop));
        if (ImGui::CollapsingHeader("Emitter"))
        {
            ImGui::Checkbox("Enabled", &enable_emitter_);
            if (enable_emitter_)
            {
                ImGuiHelper::SliderUInt("Max objects", &emitter_max_objects_count_, 0uz, 150'000uz);
            }
        }
        GUI_Tools();
    }
    ImGui::End();

    if (ImGui::Begin("Cells"))
    {
        for (const auto& [cell_index, cell_objects_count] : Enumerate(solver.cell_obj_counts_))
        {
            if (cell_objects_count != 0)
            {
                const size_t cell_x = cell_index % solver.grid_size_.x();
                const size_t cell_y = cell_index / solver.grid_size_.x();
                GuiText("Cell {} {}: {}", cell_x, cell_y, cell_objects_count);
            }
        }
    }
    ImGui::End();
}

void VerletApp::GUI_Tools()
{
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_DrawSelectedOverline;

    auto use_tool = [&](const ToolType tool_type)
    {
        if (!tool_ || tool_->GetToolType() != tool_type)
        {
            switch (tool_type)
            {
            case ToolType::SpawnObjects:
                tool_ = std::make_unique<SpawnObjectsTool>(*this);
                break;
            case ToolType::MoveObjects:
                tool_ = std::make_unique<MoveObjectsTool>(*this);
                break;
            case ToolType::DeleteObjects:
                tool_ = std::make_unique<DeleteObjectsTool>(*this);
                break;
            default:
                tool_ = nullptr;
                break;
            }
        }

        if (tool_)
        {
            tool_->DrawGUI();
        }
    };

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
}  // namespace verlet
