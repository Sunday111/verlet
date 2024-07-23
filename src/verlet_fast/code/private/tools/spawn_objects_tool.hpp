#include "tool.hpp"

namespace verlet
{
class SpawnObjectsTool : public Tool
{
public:
    using Tool::Tool;
    void Tick() override;
    void DrawGUI() override;
    ToolType GetToolType() const override
    {
        return ToolType::SpawnObjects;
    }

private:
    bool spawn_movable_objects_ = true;
    bool link_spawned_to_previous_ = false;
};
}  // namespace verlet
