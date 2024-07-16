#include "spawn_objects_tool.hpp"

#include <imgui.h>

#include "verlet_app.hpp"

namespace verlet
{

void SpawnObjectsTool::Tick()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        const auto mouse_position = app_.GetMousePositionInWorldCoordinates();
        app_.solver.objects.push_back({
            .position = mouse_position,
            .old_position = mouse_position,
            .color = edt::Math::GetRainbowColors(app_.GetTimeSeconds()),
            .movable = true,
        });

        auto& new_object = app_.solver.objects.back();
        new_object.movable = spawn_movable_objects_;
        if (link_spawned_to_previous_ && app_.solver.objects.size() > 1)
        {
            const size_t new_object_index = app_.solver.objects.size() - 1;
            const size_t previous_index = app_.solver.objects.size() - 2;
            auto& previous_object = app_.solver.objects[previous_index];
            app_.solver.links.push_back(
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

}  // namespace verlet
