#include "spawn_random_objects_tool.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdint>

#include "klvk/ui/imgui_helpers.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

void SpawnRandomObjectsTool::DrawGUI()
{
    ImGui::TextUnformatted("Fills the world with objects moving in random directions");

    uint64_t count = params_.count;
    constexpr uint64_t min_count = 0;
    const uint64_t max_count = std::max<uint64_t>(app_.max_objects_count_, count);
    if (ImGui::SliderScalar(
            "Count",
            ImGuiDataType_U64,
            &count,
            &min_count,
            &max_count,
            "%llu",
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
    {
        params_.count = static_cast<size_t>(count);
    }

    ImGui::InputScalar("Seed", ImGuiDataType_U32, &params_.seed);

    klvk::ImGuiHelper::FiniteDragFloat(
        "Max speed",
        params_.max_speed,
        0.5f,
        0.f,
        240.f,
        "%.1f world units/s",
        ImGuiSliderFlags_AlwaysClamp);
    ImGui::Checkbox("Movable", &params_.movable);
}

size_t SpawnRandomObjectsTool::Spawn()
{
    return SpawnRandomObjects(app_.solver, params_, app_.RemainingObjectBudget());
}

size_t SpawnRandomObjectsTool::ReplaceAll()
{
    app_.solver.DeleteAll();
    return SpawnRandomObjects(app_.solver, params_, app_.max_objects_count_);
}

}  // namespace verlet
