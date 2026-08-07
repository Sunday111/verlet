#include "flat_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <ranges>

#include "edt/math/math.hpp"
#include "klvk/error_handling.hpp"
#include "klvk/ui/simple_type_widget.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/object.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

FlatEmitter::FlatEmitter(const FlatEmitterConfig& in_config) : config(in_config) {}

Vec2f FlatEmitter::WorldDirection(const Vec2f& span, float length) const
{
    klvk::ErrorHandling::Ensure(
        config.direction.SquaredLength() > 0.f,
        "A flat emitter needs a direction to send objects in");

    if (!config.local_direction) return config.direction.Normalized();

    klvk::ErrorHandling::Ensure(length > 0.f, "A flat emitter with no length has no local direction to read");
    const Vec2f along = span / length;
    const Vec2f normal{-along.y(), along.x()};
    return (along * config.direction.x() + normal * config.direction.y()).Normalized();
}

void FlatEmitter::Tick(VerletApp& app)
{
    if (!enabled) return;
    if (app.solver.objects.ObjectsCount() >= app.max_objects_count_) return;

    const Vec2f start = app.RelativeToWorld(config.start);
    const Vec2f end = app.RelativeToWorld(config.end);
    const Vec2f span = end - start;
    const float length = span.Length();

    // Spacing is the gap between neighbours in object diameters, so zero puts
    // them exactly one diameter apart: touching.
    constexpr float diameter = 2 * VerletObject::GetRadius();
    const float step_length = diameter * (1.f + std::max(config.spacing, 0.f));
    // A surface too short to hold two objects still emits one, otherwise it would
    // silently produce nothing at all.
    const auto count = std::max(size_t{1}, static_cast<size_t>(length / step_length));

    const Vec2f direction = WorldDirection(span, length);

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
        c |= klvk::SimpleTypeWidget("direction", config.direction);
        c |= ImGui::Checkbox("direction is local to the surface", &config.local_direction);
        c |= klvk::SimpleTypeWidget("spacing (object diameters)", config.spacing);
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
