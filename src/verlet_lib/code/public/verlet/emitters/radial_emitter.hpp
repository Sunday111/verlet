#pragma once

#include <optional>
#include <string_view>

#include "edt/math/matrix.hpp"
#include "emitter.hpp"

namespace verlet
{

class RadialEmitterConfig
{
public:
    // Relative coordinates: -1 and 1 are the edges of the world, the origin is
    // its centre.
    edt::Vec2f position = {0, 0};
    // Relative to the shorter half of the world, so the ring stays a ring at any
    // aspect ratio. This is also what decides how many objects leave per tick.
    float radius = 0.1f;
    float phase_degrees = 0.f;
    float sector_degrees = 90.f;
    float speed_factor = 10.f;

    // Degrees per tick
    float rotation_speed = 0.f;
};

class RadialEmitterState
{
public:
    float phase_degrees = 0.f;
};

class RadialEmitter : public Emitter
{
public:
    RadialEmitter() = default;
    explicit RadialEmitter(const RadialEmitterConfig& in_config);

    void Tick(VerletApp& app) override;
    void CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const override;
    void DrawShape(const VerletApp& app, DiagnosticRenderer& renderer) const override;
    void GUI() override;
    [[nodiscard]] std::unique_ptr<Emitter> Clone() const override;
    [[nodiscard]] constexpr EmitterType GetType() const override { return EmitterType::Radial; }
    void ResetConfigurationState() override;

    [[nodiscard]] static std::optional<std::string_view> ValidateConfig(const RadialEmitterConfig& config);
    [[nodiscard]] static float NormalizeDegrees(float degrees);

    RadialEmitterConfig config{};
    RadialEmitterState state{};
};
}  // namespace verlet
