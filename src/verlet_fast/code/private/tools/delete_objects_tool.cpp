#include "delete_objects_tool.hpp"

#include <imgui.h>

#include "verlet_app.hpp"

namespace verlet
{
void DeleteObjectsTool::Tick()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        const Vec2f mouse_pos = app_.GetMousePositionInWorldCoordinates();
        const auto [erase_begin, erase_end] = std::ranges::remove_if(
            app_.solver.objects,
            [&](const VerletObject& object)
            {
                return (object.position - mouse_pos).SquaredLength() <
                       edt::Math::Sqr(delete_radius_ + object.GetRadius());
            });
        app_.solver.objects.erase(erase_begin, erase_end);
    }
}
void DeleteObjectsTool::DrawGUI()
{
    ImGui::Text("Left click to delete objects");  // NOLINT
    ImGui::SliderFloat("Delete radius", &delete_radius_, 0.1f, 100.f);
}
}  // namespace verlet
