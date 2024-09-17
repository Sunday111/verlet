#include "move_objects_tool.hpp"

#include <imgui.h>

#include "verlet/verlet_app.hpp"

namespace verlet
{
MoveObjectsTool::~MoveObjectsTool()
{
    ReleaseObject(app_.GetMousePositionInWorldCoordinates());
}

void MoveObjectsTool::Tick()
{
    auto get_mouse_pos = [opt_pos = std::optional<Vec2f>{std::nullopt}, this]() mutable -> const Vec2f&
    {
        if (!opt_pos) opt_pos = app_.GetMousePositionInWorldCoordinates();
        return *opt_pos;
    };

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        if (!std::exchange(lmb_hold, true))
        {
            if (auto id = FindObject(get_mouse_pos()); id.IsValid())
            {
                auto& object = app_.solver.objects.Get(id);
                held_object_ = {id, object.movable};
                object.movable = false;
            }
        }
    }
    else if (lmb_hold)
    {
        ReleaseObject(get_mouse_pos());
    }

    if (held_object_)
    {
        auto& object = app_.solver.objects.Get(held_object_->index);
        object.position = get_mouse_pos();
    }
}

void MoveObjectsTool::DrawGUI()
{
    ImGui::Text("Click and hold with left mouse button on object to move it");  // NOLINT
}

void MoveObjectsTool::ReleaseObject(const Vec2f& mouse_position)
{
    lmb_hold = false;
    if (held_object_)
    {
        auto& object = app_.solver.objects.Get(held_object_->index);
        object.position = mouse_position;
        object.old_position = object.position;
        object.movable = held_object_->was_movable;
        held_object_ = std::nullopt;
    }
}

ObjectId MoveObjectsTool::FindObject(const Vec2f& mouse_position) const
{
    float prev_distance_sq{};
    ObjectId result = kInvalidObjectId;

    for (const auto [id, object] : app_.solver.objects.IdentifiersAndObjects())
    {
        const float distance_sq = (object.position - mouse_position).SquaredLength();
        if (!result.IsValid() || distance_sq < prev_distance_sq)
        {
            result = id;
            prev_distance_sq = distance_sq;
        }
    }

    return result;
}
}  // namespace verlet
