#pragma once

#include "edt/math/matrix.hpp"
#include "emitter.hpp"

namespace verlet
{

class FlatEmitterConfig
{
public:
    // Ends of the emitting surface, in relative coordinates: -1 and 1 are the
    // edges of the world. Giving both ends rather than a centre and a length is
    // what lets a surface say "the whole top edge" without naming a size.
    edt::Vec2f start = {-1, 1};
    edt::Vec2f end = {1, 1};
    // Where objects go, measured the way a radial emitter's phase is: zero is
    // straight up. The surface normally lies across it.
    float direction_degrees = 180.f;
    // Distance along the surface between one spawned object and the next, in
    // world units. Object diameter packs them solid; more spreads them out and
    // emits fewer per tick. Not relative: it is a distance between objects, and
    // objects are the same size whatever the world.
    float spacing = 1.f;
    float speed_factor = 10.f;
};

class FlatEmitter : public Emitter
{
public:
    FlatEmitter() = default;
    explicit FlatEmitter(const FlatEmitterConfig& in_config);

    void Tick(VerletApp& app) override;
    void GUI() override;
    [[nodiscard]] std::unique_ptr<Emitter> Clone() const override;
    [[nodiscard]] constexpr EmitterType GetType() const override { return EmitterType::Flat; }

    FlatEmitterConfig config{};
};
}  // namespace verlet
