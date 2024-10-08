#pragma once

#include "tool.hpp"

namespace verlet
{
class DeleteObjectsTool : public Tool
{
public:
    using Tool::Tool;
    void Tick() override;
    void DrawInWorld() override;
    void DrawGUI() override;
    [[nodiscard]] ToolType GetToolType() const override { return ToolType::DeleteObjects; }

private:
    float delete_radius_ = 1.f;
};
}  // namespace verlet
