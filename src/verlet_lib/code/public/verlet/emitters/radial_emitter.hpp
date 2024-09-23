#pragma once

#include "EverydayTools/Math/Matrix.hpp"
#include "emitter.hpp"

namespace verlet
{
class RadialEmitter : public Emitter
{
public:
    void Tick(VerletApp& app) override;
    void GUI() override;
    std::unique_ptr<Emitter> Clone() const override;
    constexpr EmitterType GetType() const override { return EmitterType::Radial; }

    edt::Vec2f position = {0, 0};
    float radius = 10.f;
    float phase_degrees = 0.f;
    float sector_degrees = 90.f;
    float speed_factor = 10.f;
};
}  // namespace verlet
