#pragma once

namespace verlet
{
class VerletApp;

enum class ToolType
{
    SpawnObjects,
    MoveObjects,
    DeleteObjects
};

class Tool
{
public:
    explicit Tool(VerletApp& app) : app_(app) {}
    virtual ~Tool() = default;
    virtual void Tick() {}
    virtual void DrawInWorld() {}
    virtual void DrawGUI() {}
    virtual ToolType GetToolType() const = 0;

protected:
    VerletApp& app_;  // NOLINT
};
}  // namespace verlet
