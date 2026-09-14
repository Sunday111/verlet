#include "burst_emitter.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

#include "verlet/coloring/spawn_color/spawn_color_strategy.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
void BurstEmitter::CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const
{
    out.clear();
    if (emitted_) return;
    const size_t count = app.RemainingObjectBudget();
    if (count == 0) return;
    const auto bounds = app.solver.GetObjectBounds();
    const auto extent = bounds.Extent();
    const size_t columns =
        extent.x() > 0.f && extent.y() > 0.f
            ? std::clamp(
                  static_cast<size_t>(std::ceil(std::sqrt(static_cast<float>(count) * extent.x() / extent.y()))),
                  size_t{1},
                  count)
            : 1;
    const size_t rows = (count + columns - 1) / columns;
    const Vec2f step = extent / Vec2f{static_cast<float>(columns), static_cast<float>(rows)};
    out.reserve(count);
    for (size_t index = 0; index < count; ++index)
    {
        out.push_back(
            {bounds.Min() +
                 step * Vec2f{static_cast<float>(index % columns) + 0.5f, static_cast<float>(index / columns) + 0.5f},
             {0.f, 0.f}});
    }
}

void BurstEmitter::Tick(VerletApp& app)
{
    if (!enabled || emitted_) return;
    CollectSpawnPoints(app, spawn_points_);
    auto color = app.spawn_color_strategy_->GetColorFunction();
    for (const auto& point : spawn_points_)
    {
        auto [id, object] = app.solver.objects.Alloc();
        std::ignore = id;
        object.position = point.position;
        object.old_position = point.position;
        object.movable = true;
        object.color = color(object);
    }
    emitted_ = true;
}

void BurstEmitter::ResetConfigurationState()
{
    Emitter::ResetConfigurationState();
    emitted_ = false;
}

void BurstEmitter::GUI()
{
    ImGui::TextUnformatted("Fills the remaining object budget once, on an even grid.");
    if (ImGui::Button("Rearm")) ResetConfigurationState();
}
}  // namespace verlet
