#include "flat_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <ranges>

#include "edt/math/math.hpp"
#include "klvk/error_handling.hpp"
#include "klvk/ui/imgui_helpers.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/diagnostics/diagnostic_renderer.hpp"
#include "verlet/json/json_keys.hpp"
#include "verlet/object.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
namespace
{
constexpr float kArrowLength = 0.08f;
constexpr float kMaxSpeed = 240.f;
}  // namespace

FlatEmitter::FlatEmitter(const FlatEmitterConfig& in_config) : config(in_config) {}

std::optional<std::string_view> FlatEmitter::ValidateConfig(const FlatEmitterConfig& candidate)
{
    if (!edt::Math::IsFinite(candidate.start)) return JSONKeys::kStart;
    if (!edt::Math::IsFinite(candidate.end)) return JSONKeys::kEnd;
    if (!edt::Math::IsFinite(candidate.direction)) return JSONKeys::kDirection;
    if (!edt::Math::IsFinite(candidate.spacing) || candidate.spacing < 0.f) return JSONKeys::kSpacing;
    if (!edt::Math::IsFinite(candidate.speed_factor) || candidate.speed_factor < -kMaxSpeed ||
        candidate.speed_factor > kMaxSpeed)
    {
        return JSONKeys::kSpeedFactor;
    }
    return std::nullopt;
}

std::optional<Vec2f> FlatEmitter::WorldDirection(const Vec2f& span, float length) const
{
    if (!(config.direction.SquaredLength() > 0.f)) return std::nullopt;

    if (!config.local_direction) return config.direction.Normalized();

    if (!(length > 0.f)) return std::nullopt;
    const Vec2f along = span / length;
    const Vec2f normal{-along.y(), along.x()};
    return (along * config.direction.x() + normal * config.direction.y()).Normalized();
}

void FlatEmitter::CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const
{
    out.clear();

    const Vec2f start = app.RelativeToWorld(config.start);
    const Vec2f end = app.RelativeToWorld(config.end);
    const Vec2f span = end - start;
    const float length = span.Length();
    if (!edt::Math::IsFinite(length) || length <= 0.f) return;

    const auto direction = WorldDirection(span, length);
    if (!direction) return;

    // Spacing is the gap between neighbours in object diameters, so zero puts
    // them exactly one diameter apart: touching.
    constexpr float diameter = 2 * VerletObject::GetRadius();
    const float step_length = diameter * (1.f + std::max(config.spacing, 0.f));
    // A surface too short to hold two objects still emits one, otherwise it would
    // silently produce nothing at all.
    const size_t count = ClampSpawnPointCount(length / step_length);

    // Spawn points sit at the middle of equal shares of the surface, so they stay
    // symmetric about its centre and none lands on an end.
    const Vec2f step = span / static_cast<float>(count);

    out.reserve(count);
    for (const size_t index : std::views::iota(size_t{0}, count))
    {
        out.push_back({.position = start + step * (static_cast<float>(index) + 0.5f), .direction = *direction});
    }
}

void FlatEmitter::DrawShape(const VerletApp& app, DiagnosticRenderer& renderer) const
{
    const Vec2f start = app.RelativeToWorld(config.start);
    const Vec2f end = app.RelativeToWorld(config.end);
    renderer.DrawEmitterLine(start, end);

    const Vec2f span = end - start;
    if (const auto direction = WorldDirection(span, span.Length()))
    {
        renderer.DrawEmitterArrow(start + span / 2, *direction, app.RelativeToWorldLength(kArrowLength));
    }
}

void FlatEmitter::Tick(VerletApp& app)
{
    if (!enabled)
    {
        last_emission_was_truncated_ = false;
        return;
    }

    CollectSpawnPoints(app, spawn_points_);
    const size_t remaining = app.RemainingObjectBudget();
    const size_t spawn_count = std::min(remaining, spawn_points_.size());
    last_emission_was_truncated_ = spawn_count < spawn_points_.size();

    auto color_fn = app.spawn_color_strategy_->GetColorFunction();
    for (const size_t index : std::views::iota(size_t{0}, spawn_count))
    {
        const auto& spawn_point = spawn_points_[index];
        auto [id, object] = app.solver.objects.Alloc();
        std::ignore = id;
        object.old_position = spawn_point.position;
        object.position = spawn_point.position +
                          spawn_point.direction * (config.speed_factor * VerletSolver::kTimeSubStepDurationSeconds);
        object.movable = true;
        object.color = color_fn(object);
    }
}

void FlatEmitter::GUI()
{
    ImGui::PushID(this);
    bool changed = false;

    changed |= klvk::ImGuiHelper::FiniteDragFloat2("Start", config.start, 0.01f, -1.f, 1.f, "%.2f");
    changed |= klvk::ImGuiHelper::FiniteDragFloat2("End", config.end, 0.01f, -1.f, 1.f, "%.2f");
    changed |= klvk::ImGuiHelper::FiniteDragFloat2("Direction", config.direction, 0.01f, -1.f, 1.f, "%.2f");

    changed |= ImGui::Checkbox("Local direction", &config.local_direction);

    changed |= klvk::ImGuiHelper::FiniteDragFloat("Spacing", config.spacing, 0.05f, 0.f, 5.f, "%.2f diameters");
    config.spacing = std::max(config.spacing, 0.f);

    changed |= klvk::ImGuiHelper::FiniteDragFloat(
        "Speed",
        config.speed_factor,
        0.5f,
        -kMaxSpeed,
        kMaxSpeed,
        "%.1f world units/s",
        ImGuiSliderFlags_AlwaysClamp);

    if (changed) ResetConfigurationState();

    if (!(config.direction.SquaredLength() > 0.f))
    {
        ImGui::TextUnformatted("Emits nothing: direction is zero");
    }
    else if (!((config.end - config.start).SquaredLength() > 0.f))
    {
        ImGui::TextUnformatted("Emits nothing: surface has no length");
    }
    if (last_emission_was_truncated_) ImGui::TextUnformatted("Output limited by the object budget");
    ImGui::PopID();
}

std::unique_ptr<Emitter> FlatEmitter::Clone() const
{
    return std::make_unique<FlatEmitter>(*this);
}

}  // namespace verlet
