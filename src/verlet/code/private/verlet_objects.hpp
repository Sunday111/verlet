#pragma once

#include <vector>

#include "EverydayTools/Math/Matrix.hpp"

namespace verlet
{

struct VerletObjects
{
    VerletObjects() = default;
    VerletObjects(const VerletObjects&) = delete;
    VerletObjects(VerletObjects&&) = delete;

    std::vector<edt::Vec2f> position;
    std::vector<edt::Vec2f> old_position;
    std::vector<edt::Vec3<uint8_t>> color;
    std::vector<float> radius;

    [[nodiscard]] size_t Add()
    {
        size_t index = Size();
        position.emplace_back();
        old_position.emplace_back();
        color.push_back({{255, 0, 0}});
        radius.push_back(1.f);
        return index;
    }

    [[nodiscard]] size_t Size() const
    {
        return position.size();
    }

    [[nodiscard]] auto Indices() const
    {
        return std::views::iota(0uz, Size());
    }
};
}  // namespace verlet
