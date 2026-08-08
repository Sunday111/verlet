#include "spawn_random_objects_tool.hpp"

#include <imgui.h>

#include <algorithm>

#include "klvk/ui/imgui_helpers.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

void SpawnRandomObjectsTool::DrawGUI()
{
    ImGui::Text("Fills the world with objects going in random directions");  // NOLINT

    klvk::ImGuiHelper::SliderUInt("Count", &params_.count, size_t{1}, size_t{200'000});

    int seed = static_cast<int>(params_.seed);
    if (ImGui::InputInt("Seed", &seed)) params_.seed = static_cast<uint32_t>(std::max(0, seed));

    ImGui::SliderFloat("Max speed", &params_.max_speed, 0.f, 100.f);
    ImGui::Checkbox("Movable", &params_.movable);

    if (ImGui::Button("Spawn"))
    {
        SpawnRandomObjects(app_.solver, params_);
    }

    ImGui::SameLine();

    if (ImGui::Button("Replace all"))
    {
        app_.solver.DeleteAll();
        SpawnRandomObjects(app_.solver, params_);
    }
}

}  // namespace verlet
