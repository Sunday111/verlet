#pragma once

#include "tick_color_strategy.hpp"

namespace verlet
{
class TickColorStrategyVelocity : public TickColorStrategy
{
public:
    using TickColorStrategy::TickColorStrategy;
    ObjectColorFunction GetColorFunction() override;
    [[nodiscard]] const refl::Type& GetType() const override;
    void DrawGUI() override;

private:
    static edt::Vec4<uint8_t> Gradient(float fraction);

private:
    float red_speed_ = 20.f;
};
}  // namespace verlet

namespace refl
{

template <>
struct TypeReflectionProvider<verlet::TickColorStrategyVelocity>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return refl::StaticClassTypeInfo<verlet::TickColorStrategyVelocity>(
            "verlet::TickColorStrategyVelocity",
            edt::GUID::Create("B5DCDDC3-E1CB-4116-826D-EF169610EA2B"));
    }
};

}  // namespace refl
