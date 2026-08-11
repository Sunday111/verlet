#pragma once

#include <memory>
#include <vector>

#include "edt/math/matrix.hpp"
#include "emitter_type.hpp"

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class VerletApp;
class DiagnosticRenderer;

struct EmitterSpawnPoint
{
    Vec2f position;
    Vec2f direction;
};

class Emitter
{
public:
    static constexpr size_t kMaxSpawnPoints = 4096;

    [[nodiscard]] static constexpr size_t ClampSpawnPointCount(float estimate)
    {
        if (!(estimate > 1.f)) return 1;
        if (estimate >= static_cast<float>(kMaxSpawnPoints)) return kMaxSpawnPoints;
        return static_cast<size_t>(estimate);
    }

    virtual void Tick(VerletApp& app) = 0;
    virtual void GUI() = 0;
    [[nodiscard]] virtual constexpr EmitterType GetType() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Emitter> Clone() const = 0;
    virtual void ResetConfigurationState();
    void ResetRuntimeState();
    void PrepareClone();
    virtual ~Emitter() = default;

    virtual void CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const = 0;

    virtual void DrawShape(const VerletApp& app, DiagnosticRenderer& renderer) const = 0;

    void DrawDiagnostics(const VerletApp& app, DiagnosticRenderer& renderer) const;

    bool pending_kill = false;
    bool clone_requested = false;
    bool enabled = false;

    [[nodiscard]] bool WasLastEmissionTruncated() const { return last_emission_was_truncated_; }

protected:
    mutable std::vector<EmitterSpawnPoint> spawn_points_;
    bool last_emission_was_truncated_ = false;
};

}  // namespace verlet
