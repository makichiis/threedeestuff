#include <voxel.hpp>
#include <chunk_mesh.hpp>

ChunkMesher::ChunkMesher(const Chunk* chunk, World* world, UVOffsetScheme* s) 
    : chunk{chunk}, world{world}, uv_scheme{s} {
    static Chunk* empty_chunk = nullptr;
    if (!empty_chunk) {
        empty_chunk = new Chunk{};
    }

    if (world) {
        cache_chunk_z_front = world->get_chunk_at(chunk->position + ChunkPosition{ 0, 0, 1 });
        cache_chunk_z_back = world->get_chunk_at(chunk->position + ChunkPosition{ 0, 0, -1 });
        cache_chunk_x_right = world->get_chunk_at(chunk->position + ChunkPosition{ 1, 0, 0 });
        cache_chunk_x_left = world->get_chunk_at(chunk->position + ChunkPosition{ -1, 0, 0 });
        cache_chunk_y_top = world->get_chunk_at(chunk->position + ChunkPosition{ 0, 1, 0 });
        cache_chunk_y_bottom = world->get_chunk_at(chunk->position + ChunkPosition{ 0, -1, 0 });

        if (cache_chunk_z_front == nullptr) cache_chunk_z_front = empty_chunk;
        if (cache_chunk_z_back == nullptr) cache_chunk_z_back = empty_chunk;
        if (cache_chunk_x_right == nullptr) cache_chunk_x_right = empty_chunk;
        if (cache_chunk_x_left == nullptr) cache_chunk_x_left = empty_chunk;
        if (cache_chunk_y_top == nullptr) cache_chunk_y_top = empty_chunk;
        if (cache_chunk_y_bottom == nullptr) cache_chunk_y_bottom = empty_chunk;
    }
}

void ChunkMesh::upload_buffers() {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof vertices[0], vertices.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof indices[0], indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (void*)(3 * sizeof (float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

ChunkMesh ChunkMesher::generate_mesh()  {
    auto mesh = ChunkMesh{};
    
    // 1. pre-allocate worst-case buffer size (memory is cheap)
    mesh.vertices.reserve(Chunk::Width * Chunk::Width * Chunk::Height * 4 * 6 / 2);
    mesh.indices.reserve(Chunk::Width * Chunk::Width * Chunk::Height * 6 * 6 / 2);

    // 2. iterate all internal voxels
    for (int x = 1; x < Chunk::Width - 1; ++x) {
        for (int y = 1; y < Chunk::Height - 1; ++y) {
            for (int z = 1; z < Chunk::Width - 1; ++z) {
                if (chunk->voxels[x][y][z].type != VoxelType::NONE) add_non_edge_voxel(mesh, x, y, z);
            }
        }
    }

    // 3. iterate all edge voxels
    for (int x = 0; x < Chunk::Width; ++x) {
        for (int y = 1; y < Chunk::Height-1; ++y) {
            if (x == 0 || x == Chunk::Width-1) {
                for (int z = 1; z < Chunk::Width-1; ++z) {
                    if (chunk->voxels[x][y][z].type != VoxelType::NONE) add_voxel(mesh, x, y, z);
                }
            }

            int z1 = 0;
            int z2 = Chunk::Width-1;

            if (chunk->voxels[x][y][z1].type != VoxelType::NONE) add_voxel(mesh, x, y, z1);
            if (chunk->voxels[x][y][z2].type != VoxelType::NONE) add_voxel(mesh, x, y, z2);
        }
    }

    for (int x = 0; x < Chunk::Width; ++x) {
        for (int z = 0; z < Chunk::Width; ++z) {
            int y1 = 0;
            int y2 = Chunk::Height-1;

            if (chunk->voxels[x][y1][z].type != VoxelType::NONE) add_voxel(mesh, x, y1, z);
            if (chunk->voxels[x][y2][z].type != VoxelType::NONE) add_voxel(mesh, x, y2, z);
        }
    }

    // 4. add indices (changing this can change draw direction, btw)
    for (unsigned int i = 0; i < static_cast<unsigned int>(mesh.vertices.size()); i += 4) {
        mesh.indices.insert(mesh.indices.end(), {
            0U+i, 1U+i, 3U+i, 1U+i, 2U+i, 3U+i
        });
    }

    return mesh;
}

void ChunkMesher::add_voxel(ChunkMesh& mesh, int x, int y, int z) {
    if (!world) {
        add_voxel_single_chunk(mesh, x, y, z);
        return;
    }

    VoxelType front = (z == Chunk::Width-1) 
        ? cache_chunk_z_front->voxels[x][y][0].type : chunk->voxels[x][y][z+1].type;
    VoxelType back = (z == 0)
        ? cache_chunk_z_back->voxels[x][y][Chunk::Width-1].type : chunk->voxels[x][y][z-1].type;
    VoxelType right = (x == Chunk::Width-1)
        ? cache_chunk_x_right->voxels[0][y][z].type : chunk->voxels[x+1][y][z].type;
    VoxelType left = (x == 0)
        ? cache_chunk_x_left->voxels[Chunk::Width-1][y][z].type : chunk->voxels[x-1][y][z].type;
    VoxelType bottom = (y == 0)
        ? cache_chunk_y_bottom->voxels[x][Chunk::Height-1][z].type : chunk->voxels[x][y-1][z].type;
    VoxelType top = (y == Chunk::Height-1)
        ? cache_chunk_y_top->voxels[x][0][z].type : chunk->voxels[x][y+1][z].type;

    // TODO: perform voxel type-specific offsets pls lol

    float xf = static_cast<float>(x);
    float yf = static_cast<float>(y);
    float zf = static_cast<float>(z);

    // assume uv offset scheme is not null even though it 
    // very explicitly has a default nullptr value lol
    VoxelUV uv = uv_scheme->uvs.at(chunk->voxels[x][y][z].type);

    if (front == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf,       uv.front.top_right.u, uv.front.top_right.v },
            { xf, yf-1, zf,     uv.front.bottom_right.u, uv.front.bottom_right.v },
            { xf-1, yf-1, zf,   uv.front.bottom_left.u, uv.front.bottom_left.v },
            { xf-1, yf, zf,     uv.front.top_left.u, uv.front.top_left.v }
        });
    }
    if (back == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf-1,   uv.back.top_right.u, uv.back.top_right.v },
            { xf-1, yf-1, zf-1, uv.back.bottom_right.u, uv.back.bottom_right.v },
            { xf, yf-1, zf-1,   uv.back.bottom_left.u, uv.back.bottom_left.v },
            { xf, yf, zf-1,     uv.back.top_left.u, uv.back.top_left.v },
        });
    }
    if (right == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1,     uv.right.top_right.u, uv.right.top_right.v },
            { xf, yf-1, zf-1,   uv.right.bottom_right.u, uv.right.bottom_right.v },
            { xf, yf-1, zf,     uv.right.bottom_left.u, uv.right.bottom_left.v },
            { xf, yf, zf,       uv.right.top_left.u, uv.right.top_left.v },
        });
    }
    if (left == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf,     uv.left.top_right.u, uv.left.top_right.v },
            { xf-1, yf-1, zf,   uv.left.bottom_right.u, uv.left.bottom_right.v },
            { xf-1, yf-1, zf-1, uv.left.bottom_left.u, uv.left.bottom_left.v },
            { xf-1, yf, zf-1,   uv.left.top_left.u, uv.left.top_left.v },
        });
    }
    if (top == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1,     uv.top.top_right.u, uv.top.top_right.v },
            { xf, yf, zf,       uv.top.bottom_right.u, uv.top.bottom_right.v },
            { xf-1, yf, zf,     uv.top.bottom_left.u, uv.top.bottom_left.v },
            { xf-1, yf, zf-1,   uv.top.top_left.u, uv.top.top_left.v },
        });
    }
    if (bottom == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf-1, zf-1, uv.bottom.top_right.u, uv.bottom.top_right.v },
            { xf-1, yf-1, zf,   uv.bottom.bottom_right.u, uv.bottom.bottom_right.v },
            { xf, yf-1, zf,     uv.bottom.bottom_left.u, uv.bottom.bottom_left.v },
            { xf, yf-1, zf-1,   uv.bottom.top_left.u, uv.bottom.top_left.v },
        });
    }
}

void ChunkMesher::add_non_edge_voxel(ChunkMesh& mesh, int x, int y, int z) {
    VoxelType front = chunk->voxels[x][y][z+1].type;
    VoxelType back = chunk->voxels[x][y][z-1].type;
    VoxelType left = chunk->voxels[x-1][y][z].type;
    VoxelType right = chunk->voxels[x+1][y][z].type;
    VoxelType bottom = chunk->voxels[x][y-1][z].type;
    VoxelType top = chunk->voxels[x][y+1][z].type;

    float xf = static_cast<float>(x);
    float yf = static_cast<float>(y);
    float zf = static_cast<float>(z);

    VoxelUV uv = uv_scheme->uvs.at(chunk->voxels[x][y][z].type);

    if (front == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf,       uv.front.top_right.u, uv.front.top_right.v },
            { xf, yf-1, zf,     uv.front.bottom_right.u, uv.front.bottom_right.v },
            { xf-1, yf-1, zf,   uv.front.bottom_left.u, uv.front.bottom_left.v },
            { xf-1, yf, zf,     uv.front.top_left.u, uv.front.top_left.v }
        });
    }
    if (back == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf-1,   uv.back.top_right.u, uv.back.top_right.v },
            { xf-1, yf-1, zf-1, uv.back.bottom_right.u, uv.back.bottom_right.v },
            { xf, yf-1, zf-1,   uv.back.bottom_left.u, uv.back.bottom_left.v },
            { xf, yf, zf-1,     uv.back.top_left.u, uv.back.top_left.v },
        });
    }
    if (right == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1,     uv.right.top_right.u, uv.right.top_right.v },
            { xf, yf-1, zf-1,   uv.right.bottom_right.u, uv.right.bottom_right.v },
            { xf, yf-1, zf,     uv.right.bottom_left.u, uv.right.bottom_left.v },
            { xf, yf, zf,       uv.right.top_left.u, uv.right.top_left.v },
        });
    }
    if (left == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf,     uv.left.top_right.u, uv.left.top_right.v },
            { xf-1, yf-1, zf,   uv.left.bottom_right.u, uv.left.bottom_right.v },
            { xf-1, yf-1, zf-1, uv.left.bottom_left.u, uv.left.bottom_left.v },
            { xf-1, yf, zf-1,   uv.left.top_left.u, uv.left.top_left.v },
        });
    }
    if (top == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1,     uv.top.top_right.u, uv.top.top_right.v },
            { xf, yf, zf,       uv.top.bottom_right.u, uv.top.bottom_right.v },
            { xf-1, yf, zf,     uv.top.bottom_left.u, uv.top.bottom_left.v },
            { xf-1, yf, zf-1,   uv.top.top_left.u, uv.top.top_left.v },
        });
    }
    if (bottom == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf-1, zf-1, uv.bottom.top_right.u, uv.bottom.top_right.v },
            { xf-1, yf-1, zf,   uv.bottom.bottom_right.u, uv.bottom.bottom_right.v },
            { xf, yf-1, zf,     uv.bottom.bottom_left.u, uv.bottom.bottom_left.v },
            { xf, yf-1, zf-1,   uv.bottom.top_left.u, uv.bottom.top_left.v },
        });
    }
}

void ChunkMesher::add_voxel_single_chunk(ChunkMesh& mesh, int x, int y, int z) {
    VoxelType front = (z < Chunk::Width-1) ? chunk->voxels[x][y][z+1].type : VoxelType::NONE;
    VoxelType back = (z > 0) ? chunk->voxels[x][y][z-1].type : VoxelType::NONE;
    VoxelType left = (x > 0) ? chunk->voxels[x-1][y][z].type : VoxelType::NONE;
    VoxelType right = (x < Chunk::Width-1) ? chunk->voxels[x+1][y][z].type : VoxelType::NONE;
    VoxelType bottom = (y > 0) ? chunk->voxels[x][y-1][z].type : VoxelType::NONE;
    VoxelType top = (y < Chunk::Height-1) ? chunk->voxels[x][y+1][z].type : VoxelType::NONE;

    float xf = static_cast<float>(x);
    float yf = static_cast<float>(y);
    float zf = static_cast<float>(z);

    if (front == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf, 1.0f, 1.0f },
            { xf, yf-1, zf, 1.0f, 0.0f },
            { xf-1, yf-1, zf, 0.0f, 0.0f },
            { xf-1, yf, zf, 0.0f, 1.0f }
        });
    }
    if (back == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf-1, 1.0f, 1.0f },
            { xf-1, yf-1, zf-1, 1.0f, 0.0f },
            { xf, yf-1, zf-1, 0.0f, 0.0f },
            { xf, yf, zf-1, 0.0f, 1.0f },
        });
    }
    if (right == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1, 1.0f, 1.0f },
            { xf, yf-1, zf-1, 1.0f, 0.0f },
            { xf, yf-1, zf, 0.0f, 0.0f },
            { xf, yf, zf, 0.0f, 1.0f },
        });
    }
    if (left == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf, zf, 1.0f, 1.0f },
            { xf-1, yf-1, zf, 1.0f, 0.0f },
            { xf-1, yf-1, zf-1, 0.0f, 0.0f },
            { xf-1, yf, zf-1, 0.0f, 1.0f },
        });
    }
    if (top == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf, yf, zf-1, 1.0f, 1.0f },
            { xf, yf, zf, 1.0f, 0.0f },
            { xf-1, yf, zf, 0.0f, 0.0f },
            { xf-1, yf, zf-1, 0.0f, 1.0f },
        });
    }
    if (bottom == VoxelType::NONE) {
        mesh.vertices.insert(mesh.vertices.end(), {
            { xf-1, yf-1, zf-1, 1.0f, 1.0f },
            { xf-1, yf-1, zf, 1.0f, 0.0f },
            { xf, yf-1, zf, 0.0f, 0.0f },
            { xf, yf-1, zf-1, 0.0f, 1.0f },
        });
    }
}  