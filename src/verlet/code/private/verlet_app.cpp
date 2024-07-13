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
    objects.reserve(3000);
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

    auto spawn_with_velocity = [&](const Vec2f& position, const Vec2f& velocity, const float radius) -> VerletObject&
    {
        objects.push_back({
            .position = position,
            .old_position = solver.MakePreviousPosition(position, velocity, dt),
            .color = edt::Math::GetRainbowColors(time),
            .radius = radius,
            .movable = true,
        });
        return objects.back();
    };

    if (tool_)
    {
        tool_->Tick();
    }

    // Emitter
    if (enable_emitter_ && time - last_emit_time > 0.1f)
    {
        const Vec2f emitter_pos = world_range.Uniform({0.5, 0.85f});
        last_emit_time = time;
        constexpr float velocity_mag = 0.1f;
        constexpr float emitter_rotation_speed = 3.0f;
        const float emitter_angle = edt::kPi<float> * std::sin(time * emitter_rotation_speed) / 4;
        const Vec2f emitter_direction = edt::Math::MakeRotationMatrix(emitter_angle).MatMul(Vec2f{{0.f, -1.f}});
        spawn_with_velocity(emitter_pos, emitter_direction * velocity_mag, 2.f + std::sin(3 * time));
    }

    std::tie(last_sim_update_duration_) = edt::MeasureTime<std::chrono::milliseconds>(
        std::bind_front(&VerletSolver::Update, &solver),
        objects,
        links,
        dt);
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

    for (auto& object : objects)
    {
        const auto screen_pos = to_screen_coord(object.position);
        const auto screen_size = TransformVector(world_to_screen, object.radius + Vec2f{});
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
        ImGui::Text("%s", FormatTemp("Objects count: {}", objects.size()));                  // NOLINT
        ImGui::Text("%s", FormatTemp("Sim update duration {}", last_sim_update_duration_));  // NOLINT
        ImGui::Checkbox("Enable emitter", &enable_emitter_);
        GUI_Tools();
    }
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

    ImGui::Separator();
    ImGui::End();
}
}  // namespace verlet
