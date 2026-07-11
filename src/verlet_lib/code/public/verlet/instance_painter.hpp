#pragma once

#include <memory>

#include "EverydayTools/Math/Matrix.hpp"

namespace klvk
{
class Application;
class Texture;
class InstancedSpriteRenderer2d;
}  // namespace klvk

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

// Collects circles (position, radius, color) every frame and draws them
// in one instanced draw call through klvk.
class InstancedPainter
{
public:
    InstancedPainter();
    InstancedPainter(InstancedPainter&&) = delete;
    InstancedPainter(const InstancedPainter&) = delete;
    ~InstancedPainter();

    void Initialize(klvk::Application& app, const klvk::Texture& texture);

    void Clear();
    void DrawObject(const Vec2f& translation, const Vec4<uint8_t>& color, const Vec2f scale);

    void Render(const Mat3f& world_to_view);

private:
    std::unique_ptr<klvk::InstancedSpriteRenderer2d> renderer_;
};

}  // namespace verlet
