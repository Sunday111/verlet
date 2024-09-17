#pragma once

namespace verlet
{

class VerletApp;

class Emitter
{
public:
    virtual void Tick(VerletApp& app) = 0;
    virtual void GUI() {}

    virtual ~Emitter() = default;
};

}  // namespace verlet
