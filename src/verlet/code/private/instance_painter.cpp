#include "instance_painter.hpp"

#include "mesh_vertex.hpp"

namespace verlet
{

void InstancedPainter::Initialize()
{
    const std::array<MeshVertex, 4> vertices{
        {{.position = {1.0f, 1.0f}},
         {.position = {1.0f, -1.0f}},
         {.position = {-1.0f, -1.0f}},
         {.position = {-1.0f, 1.0f}}}};
    const std::array<uint32_t, 6> indices{0, 1, 3, 1, 2, 3};

    mesh_ = klgl::MeshOpenGL::MakeFromData<MeshVertex>(std::span{vertices}, std::span{indices});
    mesh_->Bind();
    RegisterAttribute<&MeshVertex::position>(0, false);
}

void InstancedPainter::Render()
{
    mesh_->Bind();
    for (const size_t batch_index : std::views::iota(0uz, batches_.size()))
    {
        auto& batch = batches_[batch_index];
        // number of circles initialized for the current batch
        const size_t num_locally_initialized = num_initialized_ % batch.kBatchSize;
        const size_t num_locally_used = std::min(num_circles_ - batch_index * batch.kBatchSize, batch.kBatchSize);

        // if we have new elements since last render - send color and scale for new elements
        if (num_locally_used > num_locally_initialized)
        {
            const IntRange update_range{num_locally_initialized, num_locally_used};
            batch.UpdateColorsVBO(update_range);
            batch.UpdateScaleVBO(update_range);
        }
        else
        {
            klgl::OpenGl::EnableVertexAttribArray(kColorAttribLoc);
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_color_vbo);
            klgl::OpenGl::EnableVertexAttribArray(kScaleAttribLoc);
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_scale_vbo);
        }

        // Update all offsets
        batch.UpdateTranslationsVBO({0, num_locally_used});
        mesh_->DrawInstanced(num_locally_used);
    }
}
}  // namespace verlet
