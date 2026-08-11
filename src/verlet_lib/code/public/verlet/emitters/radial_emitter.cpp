#include "radial_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

#include "edt/math/math.hpp"
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

[[nodiscard]] Vec2f BisectorDirection(float phase_degrees)
{
    return edt::Math::TransformVector(edt::Math::RotationMatrix2d(edt::Math::DegToRad(phase_degrees)), Vec2f::AxisY());
}
}  // namespace

RadialEmitter::RadialEmitter(const RadialEmitterConfig& in_config) : config(in_config)
{
    state = {.phase_degrees = NormalizeDegrees(in_config.phase_degrees)};
}

float RadialEmitter::NormalizeDegrees(float degrees)
{
    if (!edt::Math::IsFinite(degrees)) return 0.f;
    return std::remainder(degrees, 360.f);
}

std::optional<std::string_view> RadialEmitter::ValidateConfig(const RadialEmitterConfig& candidate)
{
    if (!edt::Math::IsFinite(candidate.position)) return JSONKeys::kPosition;
    if (!edt::Math::IsFinite(candidate.radius) || candidate.radius < 0.f) return JSONKeys::kRadius;
    if (!edt::Math::IsFinite(candidate.phase_degrees)) return JSONKeys::kPhaseDegrees;
    if (!edt::Math::IsFinite(candidate.sector_degrees) || candidate.sector_degrees < 0.f ||
        candidate.sector_degrees > 360.f)
    {
        return JSONKeys::kSectorDegrees;
    }
    if (!edt::Math::IsFinite(candidate.speed_factor) || candidate.speed_factor < -kMaxSpeed ||
        candidate.speed_factor > kMaxSpeed)
    {
        return JSONKeys::kSpeedFactor;
    }
    if (!edt::Math::IsFinite(candidate.rotation_speed)) return JSONKeys::kRotationSpeed;
    return std::nullopt;
}

void RadialEmitter::CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const
{
    out.clear();

    const Vec2f origin = app.RelativeToWorld(config.position);
    const float radius = app.RelativeToWorldLength(config.radius);
    if (!edt::Math::IsFinite(radius) || radius < 0.f) return;

    const float sector_radians = edt::Math::DegToRad(std::clamp(config.sector_degrees, 0.f, 360.f));
    // An emitter small enough to fit fewer than one object across still emits
    // one, otherwise it silently produces nothing at all.
    const size_t num_directions =
        ClampSpawnPointCount(sector_radians * (radius + VerletObject::GetRadius()) / (2 * VerletObject::GetRadius()));
    const float phase_radians = sector_radians / 2 + edt::Math::DegToRad(state.phase_degrees);

    out.reserve(num_directions);
    for (size_t i : std::views::iota(size_t{0}, num_directions))
    {
        auto matrix = edt::Math::RotationMatrix2d(
            phase_radians - (sector_radians * static_cast<float>(i)) / static_cast<float>(num_directions));
        auto v = edt::Math::TransformVector(matrix, Vec2f::AxisY());
        out.push_back({.position = origin + radius * v, .direction = v});
    }
}

void RadialEmitter::DrawShape(const VerletApp& app, DiagnosticRenderer& renderer) const
{
    const Vec2f origin = app.RelativeToWorld(config.position);
    const float radius = app.RelativeToWorldLength(config.radius);
    if (!edt::Math::IsFinite(radius) || radius < 0.f) return;

    renderer.DrawEmitterRing(origin, radius);

    const Vec2f direction = BisectorDirection(state.phase_degrees);
    renderer.DrawEmitterArrow(origin + direction * radius, direction, app.RelativeToWorldLength(kArrowLength));
}

void RadialEmitter::Tick(VerletApp& app)
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
                          spawn_point.direction * (config.speed_factor * VerletSolver::kTimeStepDurationSeconds);
        object.movable = true;
        object.color = color_fn(object);
    }

    state.phase_degrees = NormalizeDegrees(state.phase_degrees + config.rotation_speed);
}

void RadialEmitter::GUI()
{
    ImGui::PushID(this);
    bool changed = false;

    changed |= klvk::ImGuiHelper::FiniteDragFloat2("Position", config.position, 0.01f, -1.f, 1.f, "%.2f");

    if (klvk::ImGuiHelper::FiniteSliderFloat("Phase", config.phase_degrees, -180.f, 180.f, "%.0f deg"))
    {
        config.phase_degrees = NormalizeDegrees(config.phase_degrees);
        changed = true;
    }

    changed |= klvk::ImGuiHelper::FiniteSliderFloat(
        "Sector",
        config.sector_degrees,
        0.f,
        360.f,
        "%.0f deg",
        ImGuiSliderFlags_AlwaysClamp);

    changed |= klvk::ImGuiHelper::FiniteDragFloat("Radius", config.radius, 0.01f, 0.f, 1.f, "%.2f");
    config.radius = std::max(config.radius, 0.f);

    changed |= klvk::ImGuiHelper::FiniteDragFloat(
        "Speed",
        config.speed_factor,
        0.5f,
        -kMaxSpeed,
        kMaxSpeed,
        "%.1f world units/s",
        ImGuiSliderFlags_AlwaysClamp);

    if (klvk::ImGuiHelper::FiniteDragFloat("Rotation", config.rotation_speed, 0.1f, -10.f, 10.f, "%.1f deg/tick"))
    {
        config.rotation_speed = NormalizeDegrees(config.rotation_speed);
        changed = true;
    }

    if (changed) ResetConfigurationState();
    if (last_emission_was_truncated_) ImGui::TextUnformatted("Output limited by the object budget");
    ImGui::PopID();
}

void RadialEmitter::ResetConfigurationState()
{
    Emitter::ResetConfigurationState();
    state = {.phase_degrees = NormalizeDegrees(config.phase_degrees)};
}

std::unique_ptr<Emitter> RadialEmitter::Clone() const
{
    return std::make_unique<RadialEmitter>(*this);
}

}  // namespace verlet
