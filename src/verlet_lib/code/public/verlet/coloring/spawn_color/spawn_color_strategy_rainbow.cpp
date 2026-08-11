#include "spawn_color_strategy_rainbow.hpp"

#include "edt/math/math.hpp"
#include "klvk/ui/imgui_helpers.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
[[nodiscard]] ObjectColorFunction SpawnColorStrategyRainbow ::GetColorFunction()
{
    return [t = edt::Math::DegToRad(phase_degrees_) +
                frequency_ * GetApp().GetTimeSeconds()]([[maybe_unused]] const VerletObject& object)
    {
        auto rgb = edt::Math::GetRainbowColors(t);
        Vec4<uint8_t> c;
        c.x() = rgb.x();
        c.y() = rgb.y();
        c.z() = rgb.z();
        c.w() = 255;
        return c;
    };
}

void SpawnColorStrategyRainbow::DrawGUI()
{
    klvk::ImGuiHelper::FiniteSliderFloat("Phase", phase_degrees_, 0.f, 180.f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    klvk::ImGuiHelper::FiniteDragFloat("Frequency", frequency_, 0.05f, 0.f, 2.f, "%.2f rad/s");
    frequency_ = std::max(frequency_, 0.f);
}
}  // namespace verlet
