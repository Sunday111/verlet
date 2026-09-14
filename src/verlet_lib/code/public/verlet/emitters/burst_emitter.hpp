#pragma once

#include "emitter.hpp"

namespace verlet
{
class BurstEmitter : public Emitter
{
public:
    void Tick(VerletApp& app) override;
    void GUI() override;
    void CollectSpawnPoints(const VerletApp& app, std::vector<EmitterSpawnPoint>& out) const override;
    void DrawShape(const VerletApp&, DiagnosticRenderer&) const override {}
    void ResetConfigurationState() override;
    [[nodiscard]] constexpr EmitterType GetType() const override { return EmitterType::Burst; }
    [[nodiscard]] std::unique_ptr<Emitter> Clone() const override { return std::make_unique<BurstEmitter>(*this); }

private:
    bool emitted_ = false;
};
}  // namespace verlet
