#pragma once

#include "tick_color_strategy.hpp"

namespace verlet
{
class TickColorStrategyVelocity : public TickColorStrategy
{
public:
    using TickColorStrategy::TickColorStrategy;
    ObjectColorFunction GetColorFunction() override;
    const cppreflection::Type& GetType() const override;
    void DrawGUI() override;

private:
    static edt::Vec4<uint8_t> Gradient(float fraction);

private:
    float red_speed_ = 20.f;
};
}  // namespace verlet

namespace cppreflection
{

template <>
struct TypeReflectionProvider<verlet::TickColorStrategyVelocity>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return cppreflection::StaticClassTypeInfo<verlet::TickColorStrategyVelocity>(
            "verlet::TickColorStrategyVelocity",
            edt::GUID::Create("B5DCDDC3-E1CB-4116-826D-EF169610EA2B"));
    }
};

}  // namespace cppreflection
