#include "instance_painter.hpp"

#include <cmath>

#include "klgl/mesh/mesh_data.hpp"
#include "klgl/mesh/procedural_mesh_generator.hpp"
#include "klgl/template/register_attribute.hpp"
#include "ranges.hpp"

namespace verlet
{

InstancedPainter::InstancedPainter() = default;
InstancedPainter::~InstancedPainter() = default;

struct MeshVertex
{
    static MeshVertex FromMeshData(const klgl::GeneratedMeshData2d& data, const size_t index)
    {
        return MeshVertex{.position = data.vertices[index], .texture_coordinates = data.texture_coordinates[index]};
    }

    edt::Vec2f position{};
    edt::Vec2f texture_coordinates{};
};

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
    klgl::RegisterAttribute<&MeshVertex::position>(kVertexAttribLoc, false);
    klgl::RegisterAttribute<&MeshVertex::texture_coordinates>(kTexCoordAttribLoc, false);
}

void InstancedPainter::Render()
{
    mesh_->Bind();
    for (auto [batch_index, batch] : Enumerate(batches_))
    {
        if (num_objects_ <= batch_index * batch.kBatchSize) break;

        // number of circles initialized for the current batch
        const size_t num_locally_used = std::min(num_objects_ - batch_index * batch.kBatchSize, batch.kBatchSize);

        // Update all offsets
        batch.UpdateBuffers();
        klgl::OpenGl::EnableVertexAttribArray(kColorAttribLoc);
        klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, batch.opt_color_vbo);
        klgl::OpenGl::EnableVertexAttribArray(kScaleAttribLoc);
        klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, batch.opt_scale_vbo);
        klgl::OpenGl::EnableVertexAttribArray(kTranslationAttribLoc);
        klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, batch.opt_translation_vbo);
        mesh_->DrawInstanced(num_locally_used);
    }
}
}  // namespace verlet
