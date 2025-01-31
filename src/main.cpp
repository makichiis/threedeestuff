#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <unordered_set>
#include <iostream>
#include <sstream>
#include <chrono>

#include <voxel.hpp>
#include <rendering.hpp>
#include <chunk_mesh.hpp>
#include <input_handler.hpp>

#include <siv/PerlinNoise.hpp>

#define clamp(x, m) (x > m ? m : x)

#define MCONCAT_IMPL(x, y) x##y
#define MCONCAT(x, y) MCONCAT_IMPL(x, y)
#define static_run(expr) static void* MCONCAT(nop, __LINE__) = ([&](){ {expr;} return nullptr; })(); (void)MCONCAT(nop, __LINE__);

// VoxelType get_voxel(int x, int y, int z) {
//     //constexpr float scale = 0.005f;
//     constexpr size_t seed = 123456u;
//     static_run(srand(seed));

//     auto surface_y = 100 + rand() % 20;
//     return (y < surface_y) ? VoxelType::STONE : VoxelType::NONE;
// }

int get_voxel_height(const Chunk& chunk, int x, int z) {
    static constexpr unsigned int seed = 123456u;
    static constexpr float inv_scale = 0.0007;
    static constexpr int min_height = 100;
    
    static siv::PerlinNoise perlin{ seed };

    const double noise = perlin.octave2D(
        (x + chunk.position.x * Chunk::Width) * inv_scale,
        (z + chunk.position.z * Chunk::Width) * inv_scale, 8, 0.5);

    return clamp(min_height + abs(static_cast<int>(noise * Chunk::Height)), Chunk::Height);
}

void populate_chunk(Chunk& chunk) {
    // 123456 is my favorite seed'

    for (int x = 0; x < Chunk::Width; ++x) {
        for (int z = 0; z < Chunk::Width; ++z) {
            auto height = get_voxel_height(chunk, x, z);
            
            for (int y = 0; y < height; ++y) {
                chunk.voxels[x][y][z].type = VoxelType::STONE;
            }

            if (height > 0 && height < 260) chunk.voxels[x][height-1][z].type = VoxelType::GRASS;
            if (height > 1 && height < 272) chunk.voxels[x][height-2][z].type = VoxelType::DIRT;
            if (height > 2 && height < 272) chunk.voxels[x][height-3][z].type = VoxelType::DIRT;

            if (height == 0 || height == 1 || height == 2) { 
                chunk.voxels[x][height][z].type = VoxelType::SAND;
                for (int i = 0; i < height; ++i) {
                    chunk.voxels[x][i][z].type = VoxelType::SAND;
                }
            }
        }
    }



    // for (int x = 0; x < Chunk::Width; ++x) {
    //     for (int y = 0; y < Chunk::Height; ++y) {
    //         for (int z = 0; z < Chunk::Width; ++z) {
    //             chunk.voxels[x][y][z].type = get_voxel(x, y, z);
    //         }
    //     }
    // }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_SAMPLES, 32);

    GLFWwindow* window = glfwCreateWindow(600, 600, "threedeestuff", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to open GLFW window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    GLFWimage images[1];
    images[0].pixels = stbi_load("resources/fe_icon.png", &images[0].width, &images[0].height, 0, 4);
    if (!images[0].pixels) {
        std::cout << "failed to load icon.\n";
        return 1;
    }
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD bindings for OpenGL.\n";
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    //glEnable(GL_MULTISAMPLE);

    glCullFace(GL_FRONT);
    glViewport(0, 0, 600, 600);

    // Chunk stuff 
    std::cout << "Loading chunk...\n";

    auto start = std::chrono::system_clock::now();

    // auto chunk = new Chunk;
    // chunk->fill(VoxelType::STONE);
    // auto mesh = ChunkMesher(chunk, nullptr).generate_mesh();
    // mesh.upload_buffers();

    struct CoordChunkMesh {
        ChunkMesh mesh;
        ChunkPosition position;
        size_t indices;
    };
    std::vector<CoordChunkMesh> meshes;

    World world;
    for (int x = 0; x < world.world_size.x; ++x) {
        for (int y = 0; y < world.world_size.y; ++y) {
            for (int z = 0; z < world.world_size.z; ++z) {
                Chunk* chunk = new Chunk;
                chunk->position = { x, y, z };
                chunk->fill(VoxelType::NONE);
                populate_chunk(*chunk);
                
                auto key = world.get_chunk_key(chunk->position);
                world.loaded_chunks.insert({ key, chunk });
            }
        }
        std::cout << "Generated " << (x * world.world_size.y * world.world_size.z) << " chunks.\n";
    }

    auto end = std::chrono::system_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

    std::cout << "Generating meshes...\n";
    start = std::chrono::system_clock::now();
    UVOffsetScheme uv_scheme = UVOffsetScheme::with_width(64, 16);

    world.get_chunk_at({ 0, 0, 0 })->voxels[0][0][0].type = VoxelType::CRATE;

    // stupid fucking dumb mesh counter for pretty printing
    int counter = 0;
    
    for (int x = 0; x < world.world_size.x; ++x) {
        for (int y = 0; y < world.world_size.y; ++y) {
            for (int z = 0; z < world.world_size.z; ++z) {
                Chunk* chunk = world.loaded_chunks.at(world.get_chunk_key(ChunkPosition{ x, y, z }));
                if (!chunk) {
                    std::cout << "Chunk at position " << x << ' ' << y << ' ' << z << " not found.\n";
                    continue;
                }
                
                auto mesh = ChunkMesher(chunk, &world, &uv_scheme).generate_mesh();
                mesh.upload_buffers();
                auto size = mesh.indices.size();
                mesh.indices.clear();
                mesh.vertices.clear();
                meshes.push_back({ mesh, chunk->position, size });

                ++counter;
                if (counter % (world.world_size.y * world.world_size.z) == 0) {
                    std::cout << "Generated " << counter << " meshes.\n";
                }
            }
        }
    }

    std::cout << "World remaining in memory.\n";
    // std::cout << "Freeing all chunks...\n";
    // for (auto& chunk_node : world.loaded_chunks) {
    //     delete chunk_node.second;
    // }
    // world.loaded_chunks.clear();

    end = std::chrono::system_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

    std::cout << "Meshes generated.\n";

    // End of chunk stuff

    // Load texture(s)

    std::cout << "Loading textures...\n";

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("resources/atlas_64x.png", &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load image.\n";
        glfwTerminate();
        return 1;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    WindowInputHandler input_handler;
    input_handler.bind(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "Starting...\n";

    Rendering::ChunkShader shader = Rendering::ChunkShader::get_chunk_shader();

    GLuint u_lightpos = glGetUniformLocation(shader.program_id(), "u_lightpos");
    GLuint u_camerapos = glGetUniformLocation(shader.program_id(), "u_camerapos");

    // TEMP

    input_handler.camera_pos = { 50, 50, 50 };

    // two meshes
    // top mesh - water
    // bottom mesh - sand

    // both meshes need their own texture

    // water mesh is a single quad and must be translucent
    // sand mesh is a pseudo-voxel layer, and thus needs six faces

    GLuint water_vao;
    GLuint water_vbo;

    GLuint floor_vao;
    GLuint floor_vbo;

    glGenVertexArrays(1, &water_vao);
    glGenBuffers(1, &water_vbo);

    glGenVertexArrays(1, &floor_vao);
    glGenBuffers(1, &floor_vbo);

    float scale_x = world.world_size.x * Chunk::Width;
    float scale_z = world.world_size.z * Chunk::Width;

    float water_verts[6*3*2*3] = {
        0, -0.25f+106, 0, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,
        0, -0.25f+106, scale_z, 0.0f,       scale_z, 0.0f, 1.0f, 0.0f,
        scale_x, -0.25f+106, scale_z,       scale_x, scale_z, 0.0f, 1.0f, 0.0f,

        scale_x, -0.25f+106, scale_z,       scale_x, scale_z, 0.0f, 1.0f, 0.0f,
        scale_x, -0.25f+106, 0,             scale_x, 0.0f, 0.0f, 1.0f, 0.0f,
        0, -0.25f+106, 0, 0.0f,             0.0f, 0.0f, 1.0f, 0.0f,
    };

    glBindVertexArray(water_vao);
    glBindBuffer(GL_ARRAY_BUFFER, water_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof water_verts, water_verts, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(3 * sizeof (float)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(5 * sizeof (float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // water texture

    data = stbi_load("resources/water.png", &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load water texture.\n";
        glfwTerminate();
        return 1;
    }

    GLuint water_texture;
    glGenTextures(1, &water_texture);

    glBindTexture(GL_TEXTURE_2D, water_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    
    // END OF TEMP 

    while (!glfwWindowShouldClose(window)) {
        start = std::chrono::system_clock::now();
        glClearColor(0.09, 0.098, 0.114, 1.0);
        //glClearColor(0.62, 0.81, 0.93, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        input_handler.handle_framewise_key_input();

        glBindTexture(GL_TEXTURE_2D, texture);

        static bool key_r_is_pressed = false;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !key_r_is_pressed) {
            std::cout << "Rebuilding shader...\n";

            key_r_is_pressed = true;

            glDeleteProgram(shader.m_program_id);
            shader.m_program_id = 0;
            
            shader.m_program_id = Rendering::create_chunk_shader();

            shader.u_transform_loc = glGetUniformLocation(shader.program_id(), "u_transform");
            shader.u_model_loc = glGetUniformLocation(shader.program_id(), "u_model");
            u_lightpos = glGetUniformLocation(shader.m_program_id, "u_lightpos");
            u_camerapos = glGetUniformLocation(shader.m_program_id, "u_camerapos");
        } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && key_r_is_pressed) {
            key_r_is_pressed = false;
        }

        glm::mat4 view = input_handler.get_projection_mat() * input_handler.get_view_mat();
        shader.use();
        shader.set_u_model(glm::identity<glm::mat4>());
        shader.set_u_transform(view);
        
        glm::vec3 light_pos = glm::vec3{ 100.0f, 200.0f, 100.0f };
        glUniform3fv(u_lightpos, 1, glm::value_ptr(light_pos));
        glUniform3fv(u_camerapos, 1, glm::value_ptr(input_handler.camera_pos));

        for (auto&& meshinfo : meshes) {
            glBindVertexArray(meshinfo.mesh.vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshinfo.mesh.ebo);

            auto pos = meshinfo.position;
            shader.set_u_model(glm::translate(glm::identity<glm::mat4>(), 
            { Chunk::Width * pos.x + 1, Chunk::Height * pos.y, Chunk::Width * pos.z + 1 }));

            glDrawElements(GL_TRIANGLES, meshinfo.indices, GL_UNSIGNED_INT, nullptr);
        }

        glCullFace(GL_BACK);
        glBindVertexArray(water_vao);
        glBindTexture(GL_TEXTURE_2D, water_texture);

        shader.set_u_model(glm::identity<glm::mat4>());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glCullFace(GL_FRONT);

        end = std::chrono::system_clock::now();
        double elapsed 
            = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f;
        double fps = 1.0f / elapsed;

        std::stringstream ss;
        ss << input_handler.camera_pos.x << " " << input_handler.camera_pos.y << " " << input_handler.camera_pos.z 
            << " | " << static_cast<int>(fps) << " fps";
        glfwSetWindowTitle(window, ss.str().data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    // delete world; // not deleting yet because i need to do a whole bunch of unload and saving stuff first 
}
