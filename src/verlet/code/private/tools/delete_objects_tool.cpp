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
        const Vec2f mouse_pos = app_.GetMousePositionInWorldCoordinates();
        auto min_cell = app_.solver.LocationToCell(mouse_pos - delete_radius_);
        auto max_cell = app_.solver.LocationToCell(mouse_pos + delete_radius_);
        const auto cell_y_range = std::views::iota(min_cell.y(), max_cell.y() + 1);

        auto rsq = edt::Math::Sqr(delete_radius_);
        for (const size_t cell_x : std::views::iota(min_cell.x(), max_cell.x() + 1))
        {
            for (const size_t cell_y : cell_y_range)
            {
                for (auto object_id : app_.solver.ForEachObjectInCell(app_.solver.CellToCellIndex({cell_x, cell_y})))
                {
                    auto& object = app_.solver.objects.Get(object_id);
                    if ((object.position - mouse_pos).SquaredLength() < rsq)
                    {
                        app_.solver.DeleteObject(object_id);
                    }
                }
            }
        }
    }
}

void DeleteObjectsTool::DrawInWorld()
{
    auto& painter = app_.GetPainter();
    painter.DrawObject(app_.GetMousePositionInWorldCoordinates(), {255, 0, 0, 127}, delete_radius_ + Vec2f{});
}

void DeleteObjectsTool::DrawGUI()
{
    ImGui::Text("Left click to delete objects");  // NOLINT
    ImGui::SliderFloat("Delete radius", &delete_radius_, 0.1f, 100.f);
    if (ImGui::Button("Delete all"))
    {
        app_.solver.DeleteAll();
    }
}
}  // namespace verlet
