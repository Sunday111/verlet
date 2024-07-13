#include "tool.hpp"

#include "verlet_app.hpp"

namespace verlet
{

void SpawnObjectsTool::Tick()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        const auto mouse_position = app_.GetMousePositionInWorldCoordinates();
        app_.world.objects.push_back({
            .position = mouse_position,
            .old_position = app_.solver.MakePreviousPosition(mouse_position, {}, app_.GetLastFrameDurationSeconds()),
            .color = edt::Math::GetRainbowColors(app_.GetTimeSeconds()),
            .movable = true,
        });

        auto& new_object = app_.world.objects.back();
        new_object.movable = spawn_movable_objects_;
        if (link_spawned_to_previous_ && app_.world.objects.size() > 1)
        {
            const size_t new_object_index = app_.world.objects.size() - 1;
            const size_t previous_index = app_.world.objects.size() - 2;
            auto& previous_object = app_.world.objects[previous_index];
            app_.world.links.push_back(
                {previous_object.GetRadius() + new_object.GetRadius(),
                 ObjectId::FromValue(previous_index),
                 ObjectId::FromValue(new_object_index)});
            if (new_object.movable)
            {
                auto dir = (new_object.position - previous_object.position).Normalized();
                new_object.position =
                    previous_object.position + dir * (previous_object.GetRadius() + new_object.GetRadius());
                new_object.old_position = new_object.position;
            }
        }
    }
}

void SpawnObjectsTool::DrawGUI()
{
    ImGui::Text("Use left mouse button to spawn objects");  // NOLINT
    ImGui::Checkbox("Spawn movable objects", &spawn_movable_objects_);
    ImGui::Checkbox("Link link to previous", &link_spawned_to_previous_);
}

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
        if (!lmb_hold)
        {
            lmb_hold = true;
            if (auto opt_index = FindObject(get_mouse_pos()))
            {
                size_t index = *opt_index;
                auto& object = app_.world.objects[index];
                held_object_ = {index, object.movable};
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
        auto& object = app_.world.objects[held_object_->index];
        object.position = get_mouse_pos();
    }
}

void MoveObjectsTool::DrawGUI()
{
    ImGui::Text("Click and hold with left mouse button on object to move it");  // NOLINT
    ImGui::Checkbox("Find closest to cursor", &find_closes_one_);
}

void MoveObjectsTool::ReleaseObject(const Vec2f& mouse_position)
{
    lmb_hold = false;
    if (held_object_)
    {
        auto& object = app_.world.objects[held_object_->index];
        object.position = mouse_position;
        object.old_position = object.position;
        object.movable = held_object_->was_movable;
        held_object_ = std::nullopt;
    }
}

std::optional<size_t> MoveObjectsTool::FindObject(const Vec2f& mouse_position) const
{
    float prev_distance_sq{};
    std::optional<size_t> opt_index;
    for (const size_t index : IndicesView(std::span{app_.world.objects}))
    {
        auto& object = app_.world.objects[index];
        const float distance_sq = (object.position - mouse_position).SquaredLength();
        if (distance_sq < select_radius_)
        {
            if (find_closes_one_)
            {
                if (!opt_index || distance_sq < prev_distance_sq)
                {
                    opt_index = index;
                }
            }
            else
            {
                opt_index.emplace(index);
                break;
            }
        }
    }

    return opt_index;
}
}  // namespace verlet
