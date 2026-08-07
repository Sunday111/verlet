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
    // Where objects go. Need not be a unit vector.
    edt::Vec2f direction = {0, -1};
    // How to read the direction. In the surface's own frame x runs start to end
    // and y is its left normal, so one direction points the same way out of
    // every surface however each one is laid out. Read in the world instead when
    // this is false.
    bool local_direction = true;
    // Gap between one spawned object and the next, in object diameters: zero
    // packs them touching, one leaves a whole object's width between them. Not
    // relative to the world, because it is a distance between objects and
    // objects are the same size whatever the world.
    float spacing = 0.f;
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

private:
    // The configured direction resolved into the world, whichever frame it was
    // written in.
    [[nodiscard]] edt::Vec2f WorldDirection(const edt::Vec2f& span, float length) const;
};
}  // namespace verlet
