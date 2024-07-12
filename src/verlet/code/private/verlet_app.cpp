#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "klgl/opengl/debug/annotations.hpp"

namespace verlet
{
void VerletApp::Initialize()
{
    Super::Initialize();
    InitializeRendering();

    start_time = Clock::now();

    objects.position.reserve(3000);
    objects.old_position.reserve(3000);
    objects.color.reserve(3000);
    objects.radius.reserve(3000);
}

void VerletApp::InitializeRendering()
{
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

void VerletApp::UpdateSimulation()
{
    const float dt = GetLastFrameDurationSeconds();
    const TimePoint current_time = Clock::now();
    const float relative_time =
        std::chrono::duration_cast<std::chrono::duration<float>>(current_time - start_time).count();

    auto spawn_at = [&](const Vec2f& position, const Vec2f& velocity, const float radius)
    {
        const size_t index = objects.Add();
        objects.position[index] = position;
        objects.old_position[index] = solver.MakePreviousPosition(position, velocity, dt);
        objects.color[index] = edt::Math::GetRainbowColors(relative_time);
        objects.radius[index] = radius;
    };

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        spawn_at(GetMousePositionInWorldCoordinates(), {0.f, 0.f}, 1.f);
    }

    // Emitter
    if (relative_time - last_emit_time > 0.1f)
    {
        last_emit_time = relative_time;
        constexpr float velocity_mag = 0.1f;
        constexpr float emitter_rotation_speed = 3.0f;
        const Vec2f direction{
            std::cos(emitter_rotation_speed * relative_time),
            std::sin(emitter_rotation_speed * relative_time)};
        spawn_at(emitter_pos, direction * velocity_mag, 2.f + std::sin(3 * relative_time));
    }

    const auto [update_duration] = edt::MeasureTime<std::chrono::milliseconds>(
        [&]
        {
            solver.Update(objects, dt);
        });
    last_sim_update_duration_ = update_duration;
}

void VerletApp::PostTick()
{
    Super::PostTick();
    const float frame_start = GetCurrentFrameStartTime();
    constexpr float target_frame_duration = (1 / 60.f) * 0.9995f;
    while (GetTimeSeconds() - frame_start < target_frame_duration)
    {
    }
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

    for (const size_t index : objects.Indices())
    {
        const auto screen_pos = to_screen_coord(objects.position[index]);
        const auto screen_size = TransformVector(world_to_screen, objects.radius[index] + Vec2f{});
        const auto& color = objects.color[index];

        circle_painter_.SetCircle(circle_index++, screen_pos, color, screen_size);
    }

    shader_->Use();
    circle_painter_.Render();
}

void VerletApp::RenderGUI()
{
    const klgl::ScopeAnnotation annotation("Render GUI");
    ImGui::Begin("Parameters");
    shader_->DrawDetails();
    ImGui::Text("%s", FormatTemp("Framerate: {}", GetFramerate()));                      // NOLINT
    ImGui::Text("%s", FormatTemp("Objects count: {}", objects.Size()));                  // NOLINT
    ImGui::Text("%s", FormatTemp("Sim update duration {}", last_sim_update_duration_));  // NOLINT
    ImGui::End();
}
}  // namespace verlet
