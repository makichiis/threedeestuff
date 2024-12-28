#include <world.hpp>

Chunk* World::get_chunk_at(ChunkPosition pos) {
    auto chunk_it = loaded_chunks.find(get_chunk_key(pos));
    
    if (chunk_it == loaded_chunks.end()) {
        return nullptr;
    }

    return chunk_it->second;
}

Voxel World::get_voxel_at(Position world_pos) const {
    auto chunk_pos = ChunkPosition::from_world_pos(world_pos);
    
    if (chunk_pos.x >= world_size.x || chunk_pos.x < -world_size.x
        || chunk_pos.y >= world_size.y || chunk_pos.y < -world_size.y
        || chunk_pos.z >= world_size.z || chunk_pos.z < -world_size.z) {
        return default_voxel;
    }

    auto key = get_chunk_key(chunk_pos);
    // if (chunk_pos.z == 11) {
    //     std::cout << "World pos: " << world_pos.x << ' ' << world_pos.y << ' ' << world_pos.z << "\n";
    //     std::cout << "Chunk key: " << key << '\n';
    // }
    auto chunk_it = loaded_chunks.find(key);
    
    if (chunk_it == loaded_chunks.end()) {
        // Note: in the future, attempt to load chunk anyway to prevent
        // remeshing when chunks load?
        return default_voxel;
    }

    int local_x = world_pos.x % Chunk::Width;
    int local_y = world_pos.y % Chunk::Height;
    int local_z = world_pos.z % Chunk::Width;

    return chunk_it->second->voxels[local_x][local_y][local_z];
}

std::string World::get_chunk_key(ChunkPosition pos) const {
    return std::to_string(pos.x) + "_" 
        + std::to_string(pos.y) + "_" 
        + std::to_string(pos.z);
}