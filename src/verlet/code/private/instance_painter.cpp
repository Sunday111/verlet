#include "instance_painter.hpp"

#include <cmath>

#include "klgl/mesh/mesh_data.hpp"
#include "klgl/mesh/procedural_mesh_generator.hpp"
#include "klgl/texture/texture.hpp"
#include "mesh_vertex.hpp"
#include "ranges.hpp"

namespace verlet
{

InstancedPainter::InstancedPainter() = default;
InstancedPainter::~InstancedPainter() = default;

void InstancedPainter::Initialize()
{
    const auto data = klgl::ProceduralMeshGenerator::GenerateQuadMesh();

    std::vector<MeshVertex> vertices;
    vertices.reserve(data.vertices.size());
    std::ranges::copy(
        RangeIndices(data.vertices) |
            std::views::transform(std::bind_front(&MeshVertex::FromMeshData, std::cref(data))),
        std::back_inserter(vertices));

    mesh_ = klgl::MeshOpenGL::MakeFromData(std::span{vertices}, std::span{data.indices}, data.topology);
    mesh_->Bind();
    RegisterAttribute<&MeshVertex::position>(kVertexAttribLoc, false);
    RegisterAttribute<&MeshVertex::texture_coordinates>(kTexCoordAttribLoc, false);

    // Generate circle mask texture
    texture_ = klgl::Texture::CreateEmpty(128, 128, GL_RGBA);
    const auto size = texture_->GetSize();
    const auto sizef = size.Cast<float>();
    std::vector<Vec4<uint8_t>> pixels;
    pixels.reserve(size.x() * size.y());

    for (const size_t y : std::views::iota(0uz, size.y()))
    {
        const float yf = (static_cast<float>(y) / sizef.y()) - 0.5f;
        for (const size_t x : std::views::iota(0uz, size.x()))
        {
            const float xf = (static_cast<float>(x) / sizef.x()) - 0.5f;
            const uint8_t opacity = (xf * xf + yf * yf < 0.25f) ? 255 : 0;
            // pixels.push_back(Vec4<uint8_t>{} + opacity);
            pixels.push_back({255, 255, 255, opacity});
        }
    }

    texture_->SetPixels(pixels);
}

void InstancedPainter::Render()
{
    mesh_->Bind();
    for (auto [batch_index, batch] : Enumerate(batches_))
    {
        if (num_circles_ <= batch_index * batch.kBatchSize) break;

        // number of circles initialized for the current batch
        const size_t num_locally_used = std::min(num_circles_ - batch_index * batch.kBatchSize, batch.kBatchSize);

        // Update all offsets
        batch.UpdateBuffers();
        klgl::OpenGl::EnableVertexAttribArray(kColorAttribLoc);
        klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_color_vbo);
        klgl::OpenGl::EnableVertexAttribArray(kScaleAttribLoc);
        klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_scale_vbo);
        klgl::OpenGl::EnableVertexAttribArray(kTranslationAttribLoc);
        klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_translation_vbo);
        mesh_->DrawInstanced(num_locally_used);
    }
}
}  // namespace verlet
