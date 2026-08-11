#pragma once

#include <memory>

#include "edt/math/matrix.hpp"

namespace klvk
{
class Application;
class Texture;
class InstancedSpriteRenderer2d;
}  // namespace klvk

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class VerletApp;

struct DiagnosticOptions
{
    bool enabled = false;
    bool show_emitters = true;
    bool show_links = true;
};

class DiagnosticRenderer
{
public:
    DiagnosticRenderer();
    DiagnosticRenderer(const DiagnosticRenderer&) = delete;
    DiagnosticRenderer(DiagnosticRenderer&&) = delete;
    ~DiagnosticRenderer();

    void Initialize(klvk::Application& app);

    void DrawEmitterRing(const Vec2f& center, float radius);
    void DrawEmitterLine(const Vec2f& from, const Vec2f& to);
    void DrawSpawnPoint(const Vec2f& center);
    void DrawEmitterArrow(const Vec2f& from, const Vec2f& direction, float length);

    void Render(const VerletApp& app, const Mat3f& world_to_view);

    DiagnosticOptions options{};

private:
    void DrawRing(const Vec2f& center, float radius, const Vec4<uint8_t>& color);
    void DrawLine(const Vec2f& from, const Vec2f& to, float thickness, const Vec4<uint8_t>& color);

    std::unique_ptr<klvk::Texture> ring_texture_;
    std::unique_ptr<klvk::Texture> line_texture_;
    std::unique_ptr<klvk::InstancedSpriteRenderer2d> rings_;
    std::unique_ptr<klvk::InstancedSpriteRenderer2d> lines_;
};

}  // namespace verlet
