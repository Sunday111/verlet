#include "tick_color_strategy_velocity.hpp"

#include "imgui.h"
#include "object.hpp"
#include "physics/verlet_solver.hpp"

namespace verlet
{

ObjectColorFunction TickColorStrategyVelocity ::GetColorFunction()
{
    return [this](const VerletObject& object)
    {
        const float speed = ((object.position - object.old_position) / VerletSolver::kTimeStepDurationSeconds).Length();
        const float fraction = std::clamp(speed / red_speed_, 0.f, 1.f);
        return Gradient(fraction);
    };
}

void TickColorStrategyVelocity::DrawGUI()
{
    ImGui::SliderFloat("Red Speed", &red_speed_, 1.f, 20.f);
}

edt::Vec3<uint8_t> TickColorStrategyVelocity::Gradient(float fraction)
{
    edt::Vec3<uint8_t> rgb{};

    if (fraction >= 0.5f)
    {
        rgb[0] = static_cast<uint8_t>(255 * (2 * fraction - 1) + 0.5f);
        rgb[1] = static_cast<uint8_t>(510 * (1 - fraction) + 0.5f);
    }
    else
    {
        rgb[1] = static_cast<uint8_t>(510 * fraction + 0.5f);
        rgb[2] = static_cast<uint8_t>(255 * (1 - 2 * fraction) + 0.5f);
    }
    return rgb;
}

const cppreflection::Type& TickColorStrategyVelocity::GetType() const
{
    return *cppreflection::GetTypeInfo<TickColorStrategyVelocity>();
}
}  // namespace verlet
