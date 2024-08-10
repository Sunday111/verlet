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

        auto rgb = edt::Math::GetRainbowColors(app_.GetTimeSeconds());

        auto [spawned_object_id, new_object] = app_.solver.objects.Alloc();
        new_object.position = mouse_position;
        new_object.old_position = mouse_position;
        new_object.color.x() = rgb.x();
        new_object.color.y() = rgb.y();
        new_object.color.z() = rgb.z();
        new_object.color.w() = 255;
        new_object.movable = spawn_movable_objects_;

        if (link_spawned_to_previous_ && previous_spawned_.IsValid())
        {
            auto& previous_object = app_.solver.objects.Get(previous_spawned_);

            const float target_distance = previous_object.GetRadius() + new_object.GetRadius();
            app_.solver.CreateLink(spawned_object_id, previous_spawned_, target_distance);

            // if spawned object is movable spawn it nearby the object it links to
            if (new_object.movable)
            {
                auto dir = (new_object.position - previous_object.position).Normalized();
                new_object.position = previous_object.position + dir * target_distance * 1.001f;
                new_object.old_position = new_object.position;
            }

            if (stabilize_chain_)
            {
                app_.solver.StabilizeChain(spawned_object_id);
            }
        }

        previous_spawned_ = spawned_object_id;
    }
}

void SpawnObjectsTool::DrawGUI()
{
    ImGui::Text("Use left mouse button to spawn objects");  // NOLINT
    ImGui::Checkbox("Spawn movable objects", &spawn_movable_objects_);
    ImGui::Checkbox("Link link to previous", &link_spawned_to_previous_);

    if (link_spawned_to_previous_)
    {
        ImGui::Checkbox("Stabilize chain", &stabilize_chain_);
    }
}

}  // namespace verlet
