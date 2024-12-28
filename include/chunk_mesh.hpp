#ifndef RL_CHUNK_MESH_HPP
#define RL_CHUNK_MESH_HPP

#include <voxel.hpp>
#include <world.hpp>

#include <glad/gl.h>

#include <vector>

// Vertex data for each voxel, with position (x, y, z) and texture coordinates (u, v)
struct Vertex {
    float x, y, z;
    float u, v;
};

// A chunk mesh. Vertex and index buffers saved here for redundancy, might
// be used later.
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
    // must be initialized through the ChunkMesh class.
    auto generate_mesh() -> ChunkMesh;
private:
    void add_voxel(ChunkMesh& mesh, int x, int y, int z);
    void add_non_edge_voxel(ChunkMesh& mesh, int x, int y, int z);

    void add_voxel_single_chunk(ChunkMesh& mesh, int x, int y, int z);

    const Chunk* chunk;
    World* world = nullptr;
    UVOffsetScheme* uv_scheme = nullptr;

    const Chunk* cache_chunk_x_left = nullptr;
    const Chunk* cache_chunk_x_right = nullptr;
    const Chunk* cache_chunk_z_front = nullptr;
    const Chunk* cache_chunk_z_back = nullptr;
    const Chunk* cache_chunk_y_bottom = nullptr;
    const Chunk* cache_chunk_y_top = nullptr;
};

#endif 