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

    klgl::OpenGl::SetClearColor(Vec4f{255, 245, 153, 255} / 255.f);
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

    std::tie(last_sim_update_duration_) =
        edt::MeasureTime<std::chrono::milliseconds>(std::bind_front(&VerletSolver::Update, &solver), world, dt);
}

void VerletApp::Render()
{
    RenderWorld();
    RenderGUI();
}

void VerletApp::RenderWorld()
{
    const klgl::ScopeAnnotation annotation("Render World");

    klgl::OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const edt::FloatRange2D<float> screen_range{.x = {-1, 1}, .y = {-1, 1}};
    const Mat3f world_to_screen = edt::Math::MakeTransform(world_range, screen_range);

    [[maybe_unused]] auto to_screen_coord = [&](const Vec2f& plot_pos)
    {
        return TransformPos(world_to_screen, plot_pos);
    };

    size_t circle_index = 0;

    // Draw constraint
    circle_painter_.SetCircle(circle_index++, Vec2f{}, {}, 2 * solver.constraint_radius / world_range.Extent());

    for (auto& object : world.objects)
    {
        const auto screen_pos = to_screen_coord(object.position);
        const auto screen_size = TransformVector(world_to_screen, object.GetRadius() + Vec2f{});
        const auto& color = object.color;

        circle_painter_.SetCircle(circle_index++, screen_pos, color, screen_size);
    }

    shader_->Use();
    circle_painter_.Render();
}

void VerletApp::RenderGUI()
{
    const klgl::ScopeAnnotation annotation("Render GUI");
    if (ImGui::Begin("Settings"))
    {
        shader_->DrawDetails();
        ImGui::Text("%s", FormatTemp("Framerate: {}", GetFramerate()));                      // NOLINT
        ImGui::Text("%s", FormatTemp("Objects count: {}", world.objects.size()));            // NOLINT
        ImGui::Text("%s", FormatTemp("Sim update duration {}", last_sim_update_duration_));  // NOLINT
        ImGui::Checkbox("Enable emitter", &enable_emitter_);
        GUI_Tools();
    }
    ImGui::End();

    if (ImGui::Begin("Cells"))
    {
        for (const auto& cell_id : std::views::keys(world.cells))
        {
            size_t objects_count = 0;
            world.ForEachObjectInCell(
                cell_id,
                [&](const ObjectId&, const VerletObject&)
                {
                    ++objects_count;
                });

            if (objects_count != 0)
            {
                ImGui::Text(  // NOLINT
                    "%s",
                    FormatTemp("Cell {} {}: {}", cell_id.GetValue().x(), cell_id.GetValue().y(), objects_count));
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
