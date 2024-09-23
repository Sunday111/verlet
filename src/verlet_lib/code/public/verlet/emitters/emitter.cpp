#include "emitter.hpp"

#include <imgui.h>

namespace verlet
{
void Emitter::DeleteButton()
{
    if (ImGui::Button("Delete"))
    {
        SetFlag(EmitterFlag::PendingKill, true);
    }
}
void Emitter::CloneButton()
{
    if (ImGui::Button("Clone"))
    {
        SetFlag(EmitterFlag::CloneRequested, true);
    }
}

void Emitter::EnabledCheckbox()
{
    bool enabled = HasFlag(EmitterFlag::Enabled);
    ImGui::Checkbox("Enabled", &enabled);
    SetFlag(EmitterFlag::Enabled, enabled);
}

}  // namespace verlet
