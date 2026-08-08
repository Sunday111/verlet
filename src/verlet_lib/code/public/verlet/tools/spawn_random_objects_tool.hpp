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

private:
    RandomObjectsParams params_{.count = 10'000, .seed = 1234, .max_speed = 10.f, .movable = true};
};
}  // namespace verlet
