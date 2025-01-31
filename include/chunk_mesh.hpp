#ifndef RL_CHUNK_MESH_HPP
#define RL_CHUNK_MESH_HPP

#include <voxel.hpp>
#include <world.hpp>

#include <siv/PerlinNoise.hpp>
#include <glad/gl.h>

#include <vector>

// Vertex data for each voxel, with position (x, y, z), texture coordinates (u, v), 
// and normals (nx, ny, nz)
struct Vertex {
    float x, y, z;
    float u, v;
    float nx, ny, nz; 
};

// A chunk mesh. Vertex and index buffers saved here for redundancy, might
// be used later. Should never be used directly by the user-facing interface.
struct ChunkMesh {
    // Initialize and upload GL buffers to the GPU.
    void upload_buffers();

    // Destroy buffers and set buffer IDs to 0.
    void destroy_buffers();

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
};

class ChunkMesher {
public:
    explicit ChunkMesher(const Chunk* chunk, World* world=nullptr, UVOffsetScheme*s=nullptr);

    // Builds a chunk mesh. Internal GL buffers are not initialized by default and
    // must be initialized after the ChunkMesh object is created. 
    // TODO: Refactor this so that ChunkMesh::create_gl_buffers() returns a low-level
    // GL buffer list, or perhaps use GL object factory to initialize meshes of various
    // types. I hate system design.
    auto generate_mesh() -> ChunkMesh;
private:
    void add_voxel(ChunkMesh& mesh, int x, int y, int z);
    void add_non_edge_voxel(ChunkMesh& mesh, int x, int y, int z);

    void add_voxel_single_chunk(ChunkMesh& mesh, int x, int y, int z);

    const Chunk* chunk;
    World* world = nullptr;

    // TODO: Move this to a GL mesher wrapper
    UVOffsetScheme* uv_scheme = nullptr;

    // Chunk caches prevent chunk lookup during meshing
    const Chunk* cache_chunk_x_left = nullptr;
    const Chunk* cache_chunk_x_right = nullptr;
    const Chunk* cache_chunk_z_front = nullptr;
    const Chunk* cache_chunk_z_back = nullptr;
    const Chunk* cache_chunk_y_bottom = nullptr;
    const Chunk* cache_chunk_y_top = nullptr;

    siv::PerlinNoise perlin{ 123456u };
};

#endif 