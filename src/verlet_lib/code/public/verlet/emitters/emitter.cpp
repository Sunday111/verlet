#include "emitter.hpp"

#include <imgui.h>

namespace verlet
{
void Emitter::DeleteButton()
{
    if (ImGui::Button("Delete"))
    {
        should_be_deleted_ = true;
    }
}

void Emitter::EnabledCheckbox()
{
    ImGui::Checkbox("Enabled", &enabled_);
}
}  // namespace verlet
