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

void RadialEmitter::Tick(VerletApp& app)
{
    if (!enabled) return;
    if (app.solver.objects.ObjectsCount() >= app.max_objects_count_) return;

    const float sector_radians = edt::Math::DegToRad(std::clamp(sector_degrees, 0.f, 360.f));
    const size_t num_directions =
        static_cast<size_t>(sector_radians * (radius + VerletObject::GetRadius()) / (2 * VerletObject::GetRadius()));
    const float phase_radians = sector_radians / 2 + edt::Math::DegToRad(phase_degrees);

    auto color_fn = app.spawn_color_strategy_->GetColorFunction();

    for (size_t i : std::views::iota(size_t{0}, num_directions))
    {
        auto matrix = edt::Math::RotationMatrix2d(
            phase_radians - (sector_radians * static_cast<float>(i)) / static_cast<float>(num_directions));
        auto v = edt::Math::TransformVector(matrix, Vec2f::AxisY());

        Vec2f old_pos = position + radius * v;
        Vec2f new_pos = position + (radius + speed_factor * VerletSolver::kTimeStepDurationSeconds) * v;

        auto [id, object] = app.solver.objects.Alloc();
        object.position = new_pos;
        object.old_position = old_pos;
        object.movable = true;
        object.color = color_fn(object);
    }
}

void RadialEmitter::GUI()
{
    if (!ImGui::CollapsingHeader("Emitter")) return;

    ImGui::Checkbox("Enabled", &enabled);
    klgl::SimpleTypeWidget("location", position);
    klgl::SimpleTypeWidget("phase degrees", phase_degrees);
    klgl::SimpleTypeWidget("sector degrees", sector_degrees);
    klgl::SimpleTypeWidget("radius", radius);
    klgl::SimpleTypeWidget("speed factor", speed_factor);
}
}  // namespace verlet
