#include "mesh_data.hpp"

#include <algorithm>
#include <array>

namespace klgl
{
MeshData MeshData::MakeIndexedQuad()
{
    const std::array<Vec2f, 4> qvertices{{{1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}, {-1.0f, 1.0f}}};
    const std::array<Vec3i, 2> qindices{{{0, 1, 3}, {1, 2, 3}}};

    MeshData data{};
    data.vertices.resize(qvertices.size());
    data.indices.resize(qindices.size());

    std::ranges::copy(qvertices, data.vertices.begin());
    std::ranges::copy(qindices, data.indices.begin());

    return data;
}

void MeshOpenGL::Bind() const
{
    OpenGl::BindVertexArray(vao);
}

void MeshOpenGL::Draw() const
{
    assert(topology == GL_TRIANGLES);
    OpenGl::DrawElements(topology, elements_count, GL_UNSIGNED_INT, nullptr);
}

void MeshOpenGL::DrawInstanced(const size_t num_instances)
{
    assert(topology == GL_TRIANGLES);
    glDrawElementsInstanced(
        GL_TRIANGLES,
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
