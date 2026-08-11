#include "spawn_color_strategy_rainbow.hpp"

#include <cmath>

#include "edt/math/math.hpp"
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
    const float old_phase = phase_degrees_;
    ImGui::SliderFloat("Phase", &phase_degrees_, 0.f, 180.f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    if (!std::isfinite(phase_degrees_)) phase_degrees_ = old_phase;

    const float old_frequency = frequency_;
    ImGui::DragFloat("Frequency", &frequency_, 0.05f, 0.f, 2.f, "%.2f rad/s");
    if (!std::isfinite(frequency_)) frequency_ = old_frequency;
    frequency_ = std::max(frequency_, 0.f);
}
}  // namespace verlet
