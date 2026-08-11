#include "diagnostic_renderer.hpp"

#include <array>
#include <cmath>
#include <vector>

#include "edt/math/math.hpp"
#include "klvk/rendering/instanced_sprite_renderer_2d.hpp"
#include "klvk/vulkan/texture.hpp"
#include "verlet/emitters/emitter.hpp"
#include "verlet/object.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
namespace
{
constexpr Vec4<uint8_t> kEmitterColor{80, 200, 255, 165};
constexpr Vec4<uint8_t> kSpawnPointColor{255, 240, 120, 220};
constexpr Vec4<uint8_t> kLinkColor{255, 90, 200, 200};

constexpr float kEmitterThickness = 0.25f;
constexpr float kLinkThickness = 0.15f;

[[nodiscard]] std::vector<uint8_t> RingMask(size_t size, float wall_share)
{
    constexpr size_t kSubSamples = 4;
    constexpr auto kSubSampleArea = static_cast<float>(kSubSamples * kSubSamples);

    const auto extent = static_cast<float>(size);
    const float outer = 0.5f;
    const float inner = outer * (1.f - wall_share);

    std::vector<uint8_t> pixels(size * size);
    for (size_t y = 0; y != size; ++y)
    {
        for (size_t x = 0; x != size; ++x)
        {
            float covered = 0;
            for (size_t sy = 0; sy != kSubSamples; ++sy)
            {
                for (size_t sx = 0; sx != kSubSamples; ++sx)
                {
                    const Vec2f sample{
                        (static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) / kSubSamples) / extent - 0.5f,
                        (static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) / kSubSamples) / extent - 0.5f};
                    const float distance = sample.Length();
                    if (distance <= outer && distance >= inner) covered += 1;
                }
            }

            pixels[x + y * size] = static_cast<uint8_t>(255.f * covered / kSubSampleArea);
        }
    }

    return pixels;
}
}  // namespace

DiagnosticRenderer::DiagnosticRenderer() = default;
DiagnosticRenderer::~DiagnosticRenderer() = default;

void DiagnosticRenderer::Initialize(klvk::Application& app)
{
    constexpr size_t kRingTextureSize = 256;
    constexpr float kRingWallShare = 0.1f;
    const auto ring = RingMask(kRingTextureSize, kRingWallShare);
    ring_texture_ =
        klvk::Texture::CreateR8(app.GetDeviceContext(), edt::Vec2<uint32_t>{} + kRingTextureSize, std::span{ring});

    constexpr std::array<uint8_t, 1> solid{255};
    line_texture_ = klvk::Texture::CreateR8(app.GetDeviceContext(), edt::Vec2<uint32_t>{} + 1, std::span{solid});

    rings_ = std::make_unique<klvk::InstancedSpriteRenderer2d>(app, *ring_texture_);
    lines_ = std::make_unique<klvk::InstancedSpriteRenderer2d>(app, *line_texture_);
}

void DiagnosticRenderer::DrawEmitterRing(const Vec2f& center, float radius)
{
    DrawRing(center, radius, kEmitterColor);
}

void DiagnosticRenderer::DrawEmitterLine(const Vec2f& from, const Vec2f& to)
{
    DrawLine(from, to, kEmitterThickness, kEmitterColor);
}

void DiagnosticRenderer::DrawSpawnPoint(const Vec2f& center)
{
    DrawRing(center, VerletObject::GetRadius(), kSpawnPointColor);
}

void DiagnosticRenderer::DrawEmitterArrow(const Vec2f& from, const Vec2f& direction, float length)
{
    if (!(length > 0.f) || direction.SquaredLength() <= 0.f) return;

    const Vec2f tip = from + direction * length;
    DrawLine(from, tip, kEmitterThickness, kEmitterColor);

    constexpr float kBarbAngle = 2.6f;
    constexpr float kBarbShare = 0.3f;
    for (const float angle : {kBarbAngle, -kBarbAngle})
    {
        const auto barb = edt::Math::TransformVector(edt::Math::RotationMatrix2d(angle), direction);
        DrawLine(tip, tip + barb * (length * kBarbShare), kEmitterThickness, kEmitterColor);
    }
}

void DiagnosticRenderer::DrawRing(const Vec2f& center, float radius, const Vec4<uint8_t>& color)
{
    rings_->Add(center, color, radius + Vec2f{});
}

void DiagnosticRenderer::DrawLine(const Vec2f& from, const Vec2f& to, float thickness, const Vec4<uint8_t>& color)
{
    const Vec2f span = to - from;
    const float length = span.Length();
    if (length <= 0.f) return;

    lines_->Add(from + span / 2, color, Vec2f{length / 2, thickness / 2}, std::atan2(span.y(), span.x()));
}

void DiagnosticRenderer::Render(const VerletApp& app, const Mat3f& world_to_view)
{
    rings_->Clear();
    lines_->Clear();

    if (!options.enabled) return;

    if (options.show_emitters)
    {
        for (const auto& emitter : app.GetEmitters()) emitter.DrawDiagnostics(app, *this);
    }

    if (options.show_links)
    {
        app.solver.ForEachLink(
            [&](ObjectId from, ObjectId to, float)
            {
                DrawLine(
                    app.solver.objects.Get(from).position,
                    app.solver.objects.Get(to).position,
                    kLinkThickness,
                    kLinkColor);
            });
    }

    lines_->Render(world_to_view);
    rings_->Render(world_to_view);
}

}  // namespace verlet
