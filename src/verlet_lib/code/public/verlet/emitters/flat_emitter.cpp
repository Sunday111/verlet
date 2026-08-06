#include "flat_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <ranges>

#include "edt/math/math.hpp"
#include "klvk/ui/simple_type_widget.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/object.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

FlatEmitter::FlatEmitter(const FlatEmitterConfig& in_config) : config(in_config) {}

void FlatEmitter::Tick(VerletApp& app)
{
    if (!enabled) return;
    if (app.solver.objects.ObjectsCount() >= app.max_objects_count_) return;

    const Vec2f start = app.RelativeToWorld(config.start);
    const Vec2f end = app.RelativeToWorld(config.end);
    const Vec2f span = end - start;
    const float length = span.Length();

    const float spacing = std::max(config.spacing, 2 * VerletObject::GetRadius());
    // A surface too short to hold two objects still emits one, otherwise it would
    // silently produce nothing at all.
    const auto count = std::max(size_t{1}, static_cast<size_t>(length / spacing));

    const auto matrix = edt::Math::RotationMatrix2d(edt::Math::DegToRad(config.direction_degrees));
    const auto direction = edt::Math::TransformVector(matrix, Vec2f::AxisY());

    // Spawn points sit at the middle of equal shares of the surface, so they stay
    // symmetric about its centre and none lands on an end.
    const Vec2f step = span / static_cast<float>(count);

    auto color_fn = app.spawn_color_strategy_->GetColorFunction();

    for (const size_t index : std::views::iota(size_t{0}, count))
    {
        const Vec2f origin = start + step * (static_cast<float>(index) + 0.5f);

        auto [id, object] = app.solver.objects.Alloc();
        object.position = origin + direction * (config.speed_factor * VerletSolver::kTimeStepDurationSeconds);
        object.old_position = origin;
        object.movable = true;
        object.color = color_fn(object);
    }
}

void FlatEmitter::GUI()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("Flat"))
    {
        DeleteButton();
        ImGui::SameLine();
        CloneButton();
        EnabledCheckbox();

        bool c = false;
        c |= klvk::SimpleTypeWidget("start", config.start);
        c |= klvk::SimpleTypeWidget("end", config.end);
        c |= klvk::SimpleTypeWidget("direction degrees", config.direction_degrees);
        c |= klvk::SimpleTypeWidget("spacing", config.spacing);
        c |= klvk::SimpleTypeWidget("speed factor", config.speed_factor);

        if (c)
        {
            ResetRuntimeState();
        }
    }
    ImGui::PopID();
}

std::unique_ptr<Emitter> FlatEmitter::Clone() const
{
    return std::make_unique<FlatEmitter>(*this);
}

}  // namespace verlet
