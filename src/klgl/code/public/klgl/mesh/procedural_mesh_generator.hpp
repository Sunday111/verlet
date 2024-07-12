#pragma once

#include <vector>

#include "EverydayTools/Math/Matrix.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/gl_api.hpp"

namespace klgl
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class ProceduralMeshGenerator
{
public:
    static std::optional<MeshData> GenerateCircleMesh(const size_t triangles_count);
    static MeshData GenerateQuadMesh();
};
}  // namespace klgl
