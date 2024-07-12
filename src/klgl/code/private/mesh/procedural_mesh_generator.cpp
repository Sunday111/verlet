#include "klgl/mesh/procedural_mesh_generator.hpp"

#include "EverydayTools/Math/Constants.hpp"
#include "EverydayTools/Math/Math.hpp"

namespace klgl
{

MeshData ProceduralMeshGenerator::GenerateQuadMesh()
{
    return {
        .vertices{
            {{1.0f, 1.0f}},    // right top
            {{1.0f, -1.0f}},   // right bottom
            {{-1.0f, -1.0f}},  // left bottom
            {{-1.0f, 1.0f}},   // left top
        },
        .indices = {0, 1, 3, 1, 2, 3},
        .topology = GL_TRIANGLES,
    };
}

std::optional<MeshData> ProceduralMeshGenerator::GenerateCircleMesh(const size_t triangles_count)
{
    if (triangles_count < 3) return std::nullopt;

    std::vector<Vec2f> vertices(triangles_count + 1);
    std::vector<uint32_t> indices(triangles_count + 2);

    const Mat2f rotation = edt::Math::MakeRotationMatrix(2 * edt::kPi<float> / static_cast<float>(triangles_count));

    // circle center
    vertices[0] = {};

    // the first one is fixed and will be rotated by some fixed degree to make other points
    vertices[1] = {0.f, 1.f};

    for (size_t i = 2; i != vertices.size(); ++i)
    {
        vertices[i] = rotation.MatMul(vertices[i - 1]);
    }

    for (uint32_t i = 0; i != indices.size(); ++i)
    {
        indices[i] = i;
    }

    indices.back() = 1;

    return {{
        .vertices = std::move(vertices),
        .indices = std::move(indices),
        .topology = GL_TRIANGLE_FAN,
    }};
}
}  // namespace klgl
