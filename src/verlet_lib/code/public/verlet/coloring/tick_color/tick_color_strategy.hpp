#pragma once

#include "CppReflection/GetTypeInfo.hpp"
#include "verlet/coloring/object_color_function.hpp"

namespace verlet
{
class VerletApp;
class VerletObject;
class TickColorStrategy
{
public:
    explicit TickColorStrategy(VerletApp& app) : app_{&app} {}
    virtual ~TickColorStrategy() = default;

    virtual ObjectColorFunction GetColorFunction() = 0;
    virtual const cppreflection::Type& GetType() const = 0;
    virtual void DrawGUI() {}

protected:
    VerletApp& GetApp() const { return *app_; }

private:
    VerletApp* app_;
};
}  // namespace verlet
