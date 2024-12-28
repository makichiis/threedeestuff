#include <voxel.hpp>

#include <cmath>

UVOffsetScheme UVOffsetScheme::with_width(int image_width, int texture_width) {
    auto scheme = UVOffsetScheme{ image_width, texture_width };

    auto base = VoxelUV::base_quad;
    auto inv_scale = static_cast<float>(image_width) / static_cast<float>(texture_width);

    scheme.uvs.insert({ VoxelType::STONE, 
        VoxelUV{
            .front = (base + UV{ 0.0f, 3.0f }) / inv_scale,
            .back = (base + UV{ 0.0f, 3.0f }) / inv_scale,
            .right = (base + UV{ 0.0f, 3.0f }) / inv_scale,
            .left = (base + UV{ 0.0f, 3.0f }) / inv_scale,
            .top = (base + UV{ 0.0f, 3.0f }) / inv_scale,
            .bottom = (base + UV{ 0.0f, 3.0f }) / inv_scale, 
        }
    });

    scheme.uvs.insert({ VoxelType::GRASS,
        VoxelUV{
            .front = (base + UV{ 2.0f, 3.0f }) / inv_scale,
            .back = (base + UV{ 2.0f, 3.0f }) / inv_scale,
            .right = (base + UV{ 2.0f, 3.0f }) / inv_scale,
            .left = (base + UV{ 2.0f, 3.0f }) / inv_scale,
            .top = (base + UV{ 1.0f, 3.0f }) / inv_scale,
            .bottom = (base + UV{ 3.0f, 3.0f }) / inv_scale
        }
    });

    scheme.uvs.insert({ VoxelType::DIRT, 
        VoxelUV{
            .front = (base + UV{ 3.0f, 3.0f }) / inv_scale, 
            .back = (base + UV{ 3.0f, 3.0f }) / inv_scale, 
            .right = (base + UV{ 3.0f, 3.0f }) / inv_scale, 
            .left = (base + UV{ 3.0f, 3.0f }) / inv_scale, 
            .top = (base + UV{ 3.0f, 3.0f }) / inv_scale, 
            .bottom = (base + UV{ 3.0f, 3.0f }) / inv_scale
        }
    });

    scheme.uvs.insert({ VoxelType::SAND, 
        VoxelUV{ 
            .front = (base + UV{ 3.0f, 1.0f }) / inv_scale,
            .back = (base + UV{ 3.0f, 1.0f }) / inv_scale,
            .right = (base + UV{ 3.0f, 1.0f }) / inv_scale,
            .left = (base + UV{ 3.0f, 1.0f }) / inv_scale,
            .top = (base + UV{ 3.0f, 1.0f }) / inv_scale,
            .bottom = (base + UV{ 3.0f, 1.0f }) / inv_scale
        } 
    });

    scheme.uvs.insert({ VoxelType::CRATE,
        VoxelUV{
            .front = (base + UV{ 0.0f, 2.0f }) / inv_scale,
            .back = (base + UV{ 0.0f, 2.0f }) / inv_scale,
            .right = (base + UV{ 0.0f, 2.0f }) / inv_scale,
            .left = (base + UV{ 0.0f, 2.0f }) / inv_scale,
            .top = (base + UV{ 1.0f, 2.0f }) / inv_scale,
            .bottom = (base + UV{ 1.0f, 2.0f }) / inv_scale
        }
    });

    return scheme;
}

ChunkPosition ChunkPosition::from_world_pos(int x, int y, int z) {
    return {
        static_cast<int>(x / Chunk::Width),
        static_cast<int>(y / Chunk::Height),
        static_cast<int>(z / Chunk::Width)
    };
}

ChunkPosition ChunkPosition::from_world_pos(Position position) {
    return {
        static_cast<int>(std::floor(static_cast<float>(position.x) / Chunk::Width)),
        static_cast<int>(std::floor(static_cast<float>(position.y) / Chunk::Height)),
        static_cast<int>(std::floor(static_cast<float>(position.z) / Chunk::Width))
    };
}

Position ChunkPosition::to_world_pos(int local_x, int local_y, int local_z) const {
    return {
        x * Chunk::Width + local_x,
        y * Chunk::Height + local_y,
        z * Chunk::Width + local_z 
    };
}

void Chunk::fill(VoxelType type) {
    for (int x = 0; x < Width; ++x) {
        for (int y = 0; y < Height; ++y) {
            for (int z = 0; z < Width; ++z) {
                voxels[x][y][z].type = type;
            }
        }
    }
}