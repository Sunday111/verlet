#include "klgl/mesh/mesh_data.hpp"

#include "klgl/error_handling.hpp"

namespace klgl
{

void MeshOpenGL::ValidateIndicesCountForTopology(const GLuint topology, const size_t num_indices)
{
    switch (topology)
    {
    case GL_TRIANGLES:
        ErrorHandling::Ensure(
            num_indices % 3 == 0,
            "Topology is GL_TRIANGLES but the number of indices is not a multiple of 3 ({} % 3 != 0)",
            num_indices);
        break;

    case GL_TRIANGLE_FAN:
        ErrorHandling::Ensure(
            num_indices > 2,
            "Topology is GL_TRIANGLE_FAN but the number of indices is less than 3 ({})",
            num_indices);
        break;

    default:
        ErrorHandling::ThrowWithMessage("Unknown topology with type {}", topology);
        break;
    }
}

void MeshOpenGL::Bind() const
{
    OpenGl::BindVertexArray(vao);
}

void MeshOpenGL::Draw() const
{
    assert(topology == GL_TRIANGLES || topology == GL_TRIANGLE_FAN);
    OpenGl::DrawElements(topology, elements_count, GL_UNSIGNED_INT, nullptr);
}

void MeshOpenGL::DrawInstanced(const size_t num_instances)
{
    assert(topology == GL_TRIANGLES || topology == GL_TRIANGLE_FAN);
    glDrawElementsInstanced(
        topology,
        static_cast<GLsizei>(elements_count),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(num_instances));
}

void MeshOpenGL::BindAndDraw() const
{
    Bind();
    Draw();
}

MeshOpenGL::~MeshOpenGL()
{
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}
}  // namespace klgl
