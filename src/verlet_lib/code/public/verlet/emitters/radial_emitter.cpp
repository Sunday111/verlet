#include "radial_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

#include "EverydayTools/Math/Math.hpp"
#include "klgl/ui/simple_type_widget.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/object.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

RadialEmitter::RadialEmitter(const RadialEmitterConfig& in_config) : config(in_config)
{
    state = {.phase_degrees = in_config.phase_degrees};
}

void RadialEmitter::Tick(VerletApp& app)
{
    if (!enabled) return;
    if (app.solver.objects.ObjectsCount() >= app.max_objects_count_) return;

    const float sector_radians = edt::Math::DegToRad(std::clamp(config.sector_degrees, 0.f, 360.f));
    const size_t num_directions = static_cast<size_t>(
        sector_radians * (config.radius + VerletObject::GetRadius()) / (2 * VerletObject::GetRadius()));
    const float phase_radians = sector_radians / 2 + edt::Math::DegToRad(state.phase_degrees);

    auto color_fn = app.spawn_color_strategy_->GetColorFunction();

    for (size_t i : std::views::iota(size_t{0}, num_directions))
    {
        auto matrix = edt::Math::RotationMatrix2d(
            phase_radians - (sector_radians * static_cast<float>(i)) / static_cast<float>(num_directions));
        auto v = edt::Math::TransformVector(matrix, Vec2f::AxisY());

        Vec2f old_pos = config.position + config.radius * v;
        Vec2f new_pos =
            config.position + (config.radius + config.speed_factor * VerletSolver::kTimeStepDurationSeconds) * v;

        auto [id, object] = app.solver.objects.Alloc();
        object.position = new_pos;
        object.old_position = old_pos;
        object.movable = true;
        object.color = color_fn(object);
    }

    state.phase_degrees += config.rotation_speed;
}

void RadialEmitter::GUI()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("Radial"))
    {
        DeleteButton();
        ImGui::SameLine();
        CloneButton();
        EnabledCheckbox();

        bool c = false;
        c |= klgl::SimpleTypeWidget("location", config.position);
        c |= klgl::SimpleTypeWidget("phase degrees", config.phase_degrees);
        c |= klgl::SimpleTypeWidget("sector degrees", config.sector_degrees);
        c |= klgl::SimpleTypeWidget("radius", config.radius);
        c |= klgl::SimpleTypeWidget("speed factor", config.speed_factor);
        c |= klgl::SimpleTypeWidget("rotation speed", config.rotation_speed);

        if (c)
        {
            ResetRuntimeState();
        }
    }
    ImGui::PopID();
}

void RadialEmitter::ResetRuntimeState()
{
    Emitter::ResetRuntimeState();
    state = {.phase_degrees = config.phase_degrees};
}

std::unique_ptr<Emitter> RadialEmitter::Clone() const
{
    return std::make_unique<RadialEmitter>(*this);
}

}  // namespace verlet
