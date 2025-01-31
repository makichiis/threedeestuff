#ifndef RL_VOXEL_HPP
#define RL_VOXEL_HPP

#include <unordered_map>
#include <iostream>
#include <cstdint>

// Voxel stuff

// Represents the type of a voxel. VoxelType::NONE refers to an
// empty voxel, or a null voxel.
enum class VoxelType : uint8_t {
    NONE,
    STONE,
    GRASS,
    DIRT,
    ROCKS,
    SAND,
    CRATE,
    STONE_BRICK,
    DOWNLOAD_CS_SOURCE,
    ORE_IRON,
    ORE_GOLD,
    ORE_COAL,
    NOT_NETHERRACK,
    FRAMEWORK,
    ORE_WEIRD,
    BOCCHER,
    GLASS
};

struct UV {
    float u;
    float v;

    UV operator+(const UV& other) const {
        return { u + other.u, v + other.v };
    }

    UV operator/(const UV& other) const {
        return { u / other.u, v / other.v };
    }

    UV operator/(const float& f) const {
        return { u / f, v / f };
    }
};

struct UVQuad {
    // UV top_right;
    // UV top_left;
    // UV bottom_left;
    // UV bottom_right;

    UV top_right;
    UV bottom_right;
    UV bottom_left;
    UV top_left;

    UVQuad operator+(const UVQuad& other) const {
        return { top_right + other.top_right, bottom_right + other.bottom_right,
                bottom_left + other.bottom_left, top_left + other.top_left };
    }

    UVQuad operator+(const UV& other) const {
        return { 
            top_right + other, bottom_right + other, bottom_left + other, top_left + other };
    }

    UVQuad operator/(const UVQuad& other) const {
        return { top_right / other.top_right, bottom_right / other.bottom_right,
            bottom_left / other.bottom_left, top_left / other.top_left };
    }

    UVQuad operator/(const float& f) const {
        return { top_right / f, bottom_right / f, bottom_left / f, top_left / f };
    }
};

struct VoxelUV {
    static constexpr UVQuad base_quad 
        = { { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f } };

    UVQuad front = base_quad;
    UVQuad back = base_quad;
    UVQuad right = base_quad;
    UVQuad left = base_quad;
    UVQuad top = base_quad;
    UVQuad bottom = base_quad;
};

// this should probably go in the renderer
struct UVOffsetScheme {
    static auto with_width(int image_width, int texture_width) -> UVOffsetScheme;

    int image_width;
    int texture_width;
    // TODO: replace with array, map is unecessary since enums can be indexes.
    std::unordered_map<VoxelType, VoxelUV> uvs;
};

// Represents a single voxel. Primitive structure used in
// voxel chunks.
struct Voxel {
    VoxelType type;
    static constexpr size_t sz = sizeof(VoxelType);
};

struct Position {
    Position(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}

    Position(std::initializer_list<int> list) {
        if (list.size() != 3) {
            throw std::invalid_argument("Initializer list must contain exactly 3 elements.");
        }
        auto it = list.begin();
        x = *it++;
        y = *it++;
        z = *it;
    }

    Position operator+(const Position& other) const {
        return Position(x + other.x, y + other.y, z + other.z);
    }

    int x;
    int y;
    int z;
};

// Represents location of chunk with respect to chunk coordinate system
// (hence the integer representation).
struct ChunkPosition {
    /**
     * @brief Retrives a chunk coordinate from global world coordinates.
     * Uses `Chunk::Width` and `Chunk::Height` for space calculation. Might
     * change in the future if chunks are ever made variable.
     */
    static auto from_world_pos(int x, int y, int z) -> ChunkPosition;

    static auto from_world_pos(Position pos) -> ChunkPosition;

    auto to_world_pos(int local_x, int local_y, int local_z) const -> Position;
    
    ChunkPosition operator+(const ChunkPosition& other) const {
        return ChunkPosition{x + other.x, y + other.y, z + other.z};
    }

    ChunkPosition operator-(const ChunkPosition& other) const {
        return ChunkPosition{x - other.x, y - other.y, z - other.z};
    }

    ChunkPosition operator*(int scalar) const {
        return ChunkPosition{x * scalar, y * scalar, z * scalar};
    }

    ChunkPosition operator/(int scalar) const {
        if (scalar != 0) {
            return ChunkPosition{x / scalar, y / scalar, z / scalar};
        } else {
            throw std::invalid_argument("Division by zero is invalid.");
        }
    }

    bool operator==(const ChunkPosition&& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    int x;
    int y;
    int z;
};



// Rectangular chunk for representing voxel formations
struct Chunk {
    static constexpr int Width = 16;
    static constexpr int Height = 256;

    /**
     * @brief Populate this chunk to comprise entirely of the passed `type`.
     */
    void fill(VoxelType type);

    Voxel voxels[Width][Height][Width] = {};
    ChunkPosition position = {};
};

#endif 