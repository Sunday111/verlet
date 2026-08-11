#include "emitter.hpp"

#include "verlet/diagnostics/diagnostic_renderer.hpp"

namespace verlet
{
void Emitter::DrawDiagnostics(const VerletApp& app, DiagnosticRenderer& renderer) const
{
    DrawShape(app, renderer);

    CollectSpawnPoints(app, spawn_points_);
    for (const auto& spawn_point : spawn_points_)
    {
        renderer.DrawSpawnPoint(spawn_point.position);
    }
}

void Emitter::ResetConfigurationState()
{
    last_emission_was_truncated_ = false;
}

void Emitter::ResetRuntimeState()
{
    pending_kill = false;
    clone_requested = false;
    ResetConfigurationState();
}

void Emitter::PrepareClone()
{
    pending_kill = false;
    clone_requested = false;
    enabled = false;
    ResetConfigurationState();
}

}  // namespace verlet
