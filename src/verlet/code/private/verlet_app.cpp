#include "verlet_app.hpp"

#include "EverydayTools/Time/MeasureTime.hpp"
#include "klgl/opengl/debug/annotations.hpp"

namespace verlet
{
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

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::GetIO().WantCaptureMouse)
    {
        auto& new_object = spawn_with_velocity(GetMousePositionInWorldCoordinates(), {0.f, 0.f}, 1.f);
        new_object.movable = spawn_movable_objects_;
        if (link_spawned_to_previous_ && objects.size() > 1)
        {
            const size_t new_object_index = objects.size() - 1;
            const size_t previous_index = objects.size() - 2;
            auto& previous_object = objects[previous_index];
            links.push_back({previous_object.radius + new_object.radius, previous_index, new_object_index});
            if (new_object.movable)
            {
                auto dir = (new_object.position - previous_object.position).Normalized();
                new_object.position = previous_object.position + dir * (previous_object.radius + new_object.radius);
                new_object.old_position = new_object.position;
            }
        }
    }

    auto get_mouse_pos = [opt_pos = std::optional<Vec2f>{std::nullopt}, this]() mutable -> const Vec2f&
    {
        if (!opt_pos) opt_pos = GetMousePositionInWorldCoordinates();
        return *opt_pos;
    };

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (!lmb_hold)
        {
            lmb_hold = true;
            for (const size_t index : IndicesView(std::span{objects}))
            {
                auto& object = objects[index];
                if ((object.position - get_mouse_pos()).SquaredLength() < 1.f)
                {
                    held_object_ = {index, object.movable};
                    object.movable = false;
                    break;
                }
            }
        }
    }
    else if (lmb_hold)
    {
        lmb_hold = false;
        if (held_object_)
        {
            auto& object = objects[held_object_->index];
            object.position = GetMousePositionInWorldCoordinates();
            object.old_position = object.position;
            object.movable = held_object_->was_movable;
            held_object_ = std::nullopt;
        }
    }

    if (held_object_)
    {
        auto& object = objects[held_object_->index];
        object.position = get_mouse_pos();
    }

    // Emitter
    if (enable_emitter_ && time - last_emit_time > 0.1f)
    {
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
    ImGui::Begin("Parameters");
    shader_->DrawDetails();
    ImGui::Text("%s", FormatTemp("Framerate: {}", GetFramerate()));                      // NOLINT
    ImGui::Text("%s", FormatTemp("Objects count: {}", objects.size()));                  // NOLINT
    ImGui::Text("%s", FormatTemp("Sim update duration {}", last_sim_update_duration_));  // NOLINT
    ImGui::Checkbox("Spawn movable objects", &spawn_movable_objects_);
    ImGui::Checkbox("Enable emitter", &enable_emitter_);
    ImGui::Checkbox("Link link to previous", &link_spawned_to_previous_);
    ImGui::End();
}
}  // namespace verlet
