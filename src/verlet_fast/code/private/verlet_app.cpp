#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "tool.hpp"

namespace verlet
{

VerletApp::VerletApp() = default;
VerletApp::~VerletApp() = default;

void VerletApp::Initialize()
{
    Super::Initialize();
    world.Reserve(3000);
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
}

void VerletApp::UpdateSimulation()
{
    const float time = GetTimeSeconds();
    const float dt = GetLastFrameDurationSeconds();

    auto spawn_with_velocity = [&](const Vec2f& position, const Vec2f& velocity) -> VerletObject&
    {
        world.objects.push_back({
            .position = position,
            .old_position = solver.MakePreviousPosition(position, velocity, dt),
            .color = edt::Math::GetRainbowColors(time),
            .movable = true,
        });
        return world.objects.back();
    };

    if (tool_)
    {
        tool_->Tick();
    }

    // Emitter
    if (enable_emitter_ && time - last_emit_time > 0.005f)
    {
        const Vec2f emitter_pos = world_range.Uniform({0.5, 0.85f});
        last_emit_time = time;
        constexpr float velocity_mag = 0.1f;
        constexpr float emitter_rotation_speed = 3.0f;
        const float emitter_angle = edt::kPi<float> * std::sin(time * emitter_rotation_speed) / 4;
        const Vec2f emitter_direction = edt::Math::MakeRotationMatrix(emitter_angle).MatMul(Vec2f{{0.f, -1.f}});
        spawn_with_velocity(emitter_pos, emitter_direction * velocity_mag);
    }

    perf_stats_.sim_update = solver.Update(world, dt);
}

void VerletApp::Render()
{
    RenderWorld();
    RenderGUI();
}

void VerletApp::RenderWorld()
{
    const klgl::ScopeAnnotation annotation("Render World");

    const edt::FloatRange2D<float> screen_range{.x = {-1, 1}, .y = {-1, 1}};
    const Mat3f world_to_screen = edt::Math::MakeTransform(world_range, screen_range);

    auto paint_instanced_object = [&, next_instance_index = 0uz](VerletObject& object) mutable
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

            perf_stats_.render.set_circle_loop =
                std::get<0>(edt::MeasureTime(std::ranges::for_each, world.objects, paint_instanced_object));

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
        GuiText("Objects count: {}", world.objects.size());
        GuiText("Sim update {}", to_flt_ms(perf_stats_.sim_update.total));
        GuiText("  Apply links {}", to_flt_ms(perf_stats_.sim_update.apply_links));
        GuiText("  Rebuild grid {}", to_flt_ms(perf_stats_.sim_update.rebuild_grid));
        GuiText("  Solve collisions {}", to_flt_ms(perf_stats_.sim_update.solve_collisions));
        GuiText("  Update positions {}", to_flt_ms(perf_stats_.sim_update.update_positions));
        GuiText("Render {}", to_flt_ms(perf_stats_.render.total));
        GuiText("  Set Circle Loop {}", to_flt_ms(perf_stats_.render.set_circle_loop));
        ImGui::Checkbox("Enable emitter", &enable_emitter_);
        GUI_Tools();
    }
    ImGui::End();

    if (ImGui::Begin("Cells"))
    {
        for (const size_t cell_index : std::views::iota(0uz, world.cells.size()))
        {
            if (const auto objects_count = world.cell_obj_counts_[cell_index]; objects_count != 0)
            {
                const size_t cell_x = cell_index % world.grid_size_.x();
                const size_t cell_y = cell_index / world.grid_size_.x();
                GuiText("Cell {} {}: {}", cell_x, cell_y, objects_count);
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
            ImGui::Text("Left click to delete objects");  // NOLINT
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}  // namespace verlet
