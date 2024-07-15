#include "instance_painter.hpp"

#include <cmath>

#include "klgl/mesh/procedural_mesh_generator.hpp"
#include "mesh_vertex.hpp"
#include "ranges.hpp"

namespace verlet
{

void InstancedPainter::Initialize()
{
    const auto [vertices, indices, topology] = *klgl::ProceduralMeshGenerator::GenerateCircleMesh(20);
    mesh_ = klgl::MeshOpenGL::MakeFromData(std::span{vertices}, std::span{indices}, topology);
    mesh_->Bind();
    RegisterAttribute<&MeshVertex::position>(0, false);
}

void InstancedPainter::Render()
{
    mesh_->Bind();
    for (auto [batch_index, batch] : Enumerate(batches_))
    {
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
