#include "mesh_vertex.hpp"

#include "klgl/mesh/mesh_data.hpp"

namespace verlet
{
MeshVertex MeshVertex::FromMeshData(const klgl::MeshData& data, const size_t index)
{
    return MeshVertex{.position = data.vertices[index], .texture_coordinates = data.texture_coordinates[index]};
}

}  // namespace verlet
