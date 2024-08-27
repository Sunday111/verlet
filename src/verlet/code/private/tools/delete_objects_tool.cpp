#include "delete_objects_tool.hpp"

#include <imgui.h>

#include "fmt/ranges.h"  // IWYU pragma: keep
#include "verlet_app.hpp"

namespace verlet
{
void DeleteObjectsTool::Tick()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
    {
        std::vector<ObjectId> to_delete;
        const Vec2f mouse_pos = app_.GetMousePositionInWorldCoordinates();

        auto get_id = [](const std::tuple<ObjectId, VerletObject&>& kv)
        {
            return std::get<0>(kv);
        };

        auto filter_by_distance = VerletSolver::ObjectFilters::InArea(mouse_pos, delete_radius_);
        auto delete_object = std::bind_front(&VerletSolver::DeleteObject, &app_.solver);

        std::ranges::for_each(
            app_.solver.objects.IdentifiersAndObjects() | filter_by_distance | std::views::transform(get_id),
            delete_object);
    }
}

void DeleteObjectsTool::DrawInWorld()
{
    auto& painter = app_.GetPainter();
    // const auto mouse_pos = app_.GetMousePositionInWorldCoordinates();
    // const auto screen_pos = edt::Math::TransformPos(app_.GetWorldToViewTransform(), mouse_pos);
    // const auto screen_size = edt::Math::TransformVector(app_.GetWorldToViewTransform(), delete_radius_ + Vec2f{});
    painter.DrawObject(app_.GetMousePositionInWorldCoordinates(), {255, 0, 0, 127}, delete_radius_ + Vec2f{});
}

void DeleteObjectsTool::DrawGUI()
{
    ImGui::Text("Left click to delete objects");  // NOLINT
    ImGui::SliderFloat("Delete radius", &delete_radius_, 0.1f, 100.f);
}
}  // namespace verlet
