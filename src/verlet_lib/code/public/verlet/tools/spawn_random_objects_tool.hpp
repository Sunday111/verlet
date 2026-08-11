#pragma once

#include "tool.hpp"
#include "verlet/random_objects.hpp"

namespace verlet
{
class SpawnRandomObjectsTool : public Tool
{
public:
    using Tool::Tool;
    void DrawGUI() override;
    [[nodiscard]] ToolType GetToolType() const override { return ToolType::SpawnRandomObjects; }

    [[nodiscard]] RandomObjectsParams& GetParams() { return params_; }
    [[nodiscard]] const RandomObjectsParams& GetParams() const { return params_; }
    size_t Spawn();
    size_t ReplaceAll();

private:
    RandomObjectsParams params_{.count = 10'000, .seed = 1234, .max_speed = 10.f, .movable = true};
};
}  // namespace verlet
