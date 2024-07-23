#pragma once

#include "CppReflection/GetTypeInfo.hpp"
#include "coloring/object_color_function.hpp"

namespace verlet
{
class VerletApp;
class SpawnColorStrategy
{
public:
    explicit SpawnColorStrategy(const VerletApp& app) : app_{&app} {}
    virtual ~SpawnColorStrategy() = default;
    [[nodiscard]] virtual ObjectColorFunction GetColorFunction() = 0;
    [[nodiscard]] virtual const cppreflection::Type& GetType() const = 0;
    virtual void DrawGUI() {}

    [[nodiscard]] const VerletApp& GetApp() const
    {
        return *app_;
    }

private:
    const VerletApp* app_ = nullptr;
};
}  // namespace verlet
