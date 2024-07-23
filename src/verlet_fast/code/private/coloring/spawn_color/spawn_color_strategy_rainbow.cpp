#include "spawn_color_strategy_rainbow.hpp"

#include "verlet_app.hpp"

namespace verlet
{
[[nodiscard]] ObjectColorFunction SpawnColorStrategyRainbow ::GetColorFunction()
{
    return [t = phase_ + frequency_ * GetApp().GetTimeSeconds()]([[maybe_unused]] const VerletObject& object)
    {
        return edt::Math::GetRainbowColors(t);
    };
}

void SpawnColorStrategyRainbow::DrawGUI()
{
    ImGui::SliderFloat("Phase", &phase_, -10.f, 10.f);
    ImGui::SliderFloat("Frequency", &frequency_, 0.f, 2.f);
}
}  // namespace verlet
