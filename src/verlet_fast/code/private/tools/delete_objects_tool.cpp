#include "delete_objects_tool.hpp"

#include <imgui.h>

#include "verlet_app.hpp"

namespace verlet
{
void DeleteObjectsTool::Tick()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        std::vector<ObjectId> to_delete;
        const Vec2f mouse_pos = app_.GetMousePositionInWorldCoordinates();

        for (ObjectId id : app_.solver.objects.AllObjects())
        {
            auto& object = app_.solver.objects.Get(id);
            if ((object.position - mouse_pos).SquaredLength() < edt::Math::Sqr(delete_radius_ + object.GetRadius()))
            {
                to_delete.push_back(id);
            }
        }

        for (ObjectId id : to_delete)
        {
            app_.solver.objects.Free(id);
        }
    }
}

void DeleteObjectsTool::DrawGUI()
{
    ImGui::Text("Left click to delete objects");  // NOLINT
    ImGui::SliderFloat("Delete radius", &delete_radius_, 0.1f, 100.f);
}
}  // namespace verlet
