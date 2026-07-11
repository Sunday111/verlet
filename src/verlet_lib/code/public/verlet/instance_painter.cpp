#include "instance_painter.hpp"

#include "klvk/rendering/instanced_sprite_renderer_2d.hpp"

namespace verlet
{

InstancedPainter::InstancedPainter() = default;
InstancedPainter::~InstancedPainter() = default;

void InstancedPainter::Initialize(klvk::Application& app, const klvk::Texture& texture)
{
    renderer_ = std::make_unique<klvk::InstancedSpriteRenderer2d>(app, texture);
}

void InstancedPainter::Clear()
{
    renderer_->Clear();
}

void InstancedPainter::DrawObject(const Vec2f& translation, const Vec4<uint8_t>& color, const Vec2f scale)
{
    renderer_->Add(translation, color, scale);
}

void InstancedPainter::Render(const Mat3f& world_to_view)
{
    renderer_->Render(world_to_view);
}

}  // namespace verlet
