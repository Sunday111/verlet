#include <optional>

#include "EverydayTools/Math/Matrix.hpp"
#include "object.hpp"
#include "tool.hpp"

namespace verlet
{
using namespace edt::lazy_matrix_aliases;  // NOLINT

class MoveObjectsTool : public Tool
{
public:
    struct HeldObject
    {
        ObjectId index{};
        bool was_movable{};
    };

    using Tool::Tool;
    ~MoveObjectsTool() override;
    void Tick() override;
    void DrawGUI() override;
    ToolType GetToolType() const override
    {
        return ToolType::MoveObjects;
    }

private:
    void ReleaseObject(const Vec2f& mouse_position);
    ObjectId FindObject(const Vec2f& mouse_position) const;

private:
    bool lmb_hold = false;
    std::optional<HeldObject> held_object_;
};
}  // namespace verlet
