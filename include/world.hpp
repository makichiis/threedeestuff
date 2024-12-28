#ifndef RL_WORLD_HPP
#define RL_WORLD_HPP

#include <voxel.hpp>

#include <unordered_map>
#include <string>

struct World {
    /**
     * @brief World size in chunks.
     */
    struct WorldSize {
        int x = 128;
        int y = 1;
        int z = 128;
    } world_size; // TODO: Crash during worldgen (not meshing) when x = 256
 
    auto get_chunk_at(ChunkPosition pos) -> Chunk*;

    /**
     * @brief Retrieve a reference to a voxel at the provided
     * `world_pos`. If the provided position falls out of the
     * bounds of the world, a reference to a default `VoxelType::NONE`
     * voxel is returned.
     */
    auto get_voxel_at(Position world_pos) const -> Voxel;

    std::unordered_map<std::string, Chunk*> loaded_chunks;

    auto get_chunk_key(ChunkPosition pos) const -> std::string;
private:
    static constexpr Voxel default_voxel{ VoxelType::NONE };
};

#endif 