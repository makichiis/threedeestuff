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

void populate_chunk(Chunk& chunk) {
    // 123456 is my favorite seed

    // // for (int x = 0; x < Chunk::Width; ++x) {
    // //     for (int y = 0; y < Chunk::Height; ++y) {
    // //         if (x == 0 || x == Chunk::Width - 1) {
    // //             for (int z = 0; z < Chunk::Width; ++z) {
    // //                 chunk.voxels[x][y][z].type = VoxelType::NONE;
    // //             }
    // //         }

    // //         chunk.voxels[x][y][0].type = VoxelType::NONE;
    // //         chunk.voxels[x][y][Chunk::Width-1].type = VoxelType::NONE;
    // //     }
    // // }

    static constexpr unsigned int seed = 1u;
    static siv::PerlinNoise perlin{ seed };

    for (int x = 0; x < Chunk::Width; ++x) {
        for (int z = 0; z < Chunk::Width; ++z) {
            const double noise = perlin.octave2D(
                ((x + chunk.position.x * Chunk::Width) * 0.004), 
                ((z + chunk.position.z * Chunk::Width) * 0.004), 5);

            int height = static_cast<int>(noise * Chunk::Height);
            for (int y = 0; y < height-1; ++y) {
                chunk.voxels[x][y][z].type = VoxelType::STONE;
            }

            if (height > 0 && height < 54) chunk.voxels[x][height-1][z].type = VoxelType::GRASS;
            if (height > 1 && height < 64) chunk.voxels[x][height-2][z].type = VoxelType::DIRT;
            if (height > 2 && height < 64) chunk.voxels[x][height-3][z].type = VoxelType::DIRT;

            if (height == 0 || height == 1 || height == 2) 
                chunk.voxels[x][height][z].type = VoxelType::SAND;
        }
    }
}

int main() { 
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "threedeestuff", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to open GLFW window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD bindings for OpenGL.\n";
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    glCullFace(GL_FRONT);
    glViewport(0, 0, 600, 600);

    // Chunk stuff 
    std::cout << "Loading chunk...\n";

    auto start = std::chrono::system_clock::now();

    // auto chunk = new Chunk;
    // chunk->fill(VoxelType::STONE);
    // auto mesh = ChunkMesher(chunk, nullptr).generate_mesh();
    // mesh.upload_buffers();

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

    struct CoordChunkMesh {
        ChunkMesh mesh;
        ChunkPosition position;
    };

    std::cout << "Generating meshes...\n";
    start = std::chrono::system_clock::now();
    UVOffsetScheme uv_scheme = UVOffsetScheme::with_width(64, 16);

    world.get_chunk_at({ 0, 0, 0 })->voxels[0][0][0].type = VoxelType::CRATE;

    int counter = 0;
    std::vector<CoordChunkMesh> meshes;
    for (int x = 0; x < world.world_size.x; ++x) {
        for (int y = 0; y < world.world_size.y; ++y) {
            for (int z = 0; z < world.world_size.z; ++z) {
                Chunk* chunk = world.loaded_chunks.at(world.get_chunk_key(ChunkPosition{ x, y, z }));
                if (!chunk) {
                    std::cout << "Chunk at position " << x << ' ' << y << ' ' << z << " not found.\n";
                    continue;
                }
                
                // std::cout << "{ " 
                //     << chunk->position.x << ", " << chunk->position.y << ", " << chunk->position.z << " }\n";
                auto mesh = ChunkMesher(chunk, &world, &uv_scheme).generate_mesh();
                mesh.upload_buffers();
                meshes.push_back({ mesh, chunk->position });

                ++counter;
                if (counter % (world.world_size.y * world.world_size.z) == 0) {
                    std::cout << "Generated " << counter << " meshes.\n";
                }
            }
        }
    }

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

    // temporary controls for isometric camera.
    // TODO: Make isometric camera?
    float pos = 16.0f;
    float speed = 0.1f;
    float pos_y = 0.0f;
    glm::mat4 view = glm::lookAt(glm::vec3{ pos, pos + pos_y, -pos }, { 0.f, pos_y, 0.f }, { 0.f, 1.f, 0.f });

    std::cout << "Starting...\n";

    Rendering::ChunkShader shader = Rendering::ChunkShader::get_chunk_shader();

    

    while (!glfwWindowShouldClose(window)) {
        start = std::chrono::system_clock::now();
        glClearColor(0.70, 0.85, 0.95, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        input_handler.handle_framewise_key_input();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // temporary controls for isometric camera
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            pos -= speed * abs(pos / 5) + speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            pos += speed * abs(pos / 5) + speed;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            pos_y += speed * abs(pos / 5) + speed;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            pos_y -= speed * abs(pos / 5) + speed;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            pos = 16.0f;
            pos_y = 0.0f;
        }

        glBindTexture(GL_TEXTURE_2D, texture);

        view = glm::lookAt(glm::vec3{pos, pos + pos_y, -pos}, {0.f, pos_y, 0.f}, {0.f, 1.f, 0.f});
        // TODO: Move overriden controls to isometric camera object
        // renderer.draw(input_handler.get_projection_mat() * view);

        // auto camera_view = input_handler.get_projection_mat() * input_handler.get_view_mat();
        // for (auto&& renderer : chunk_renderers) {
        //     renderer.draw(camera_view);
        // }

        // auto camera_view = input_handler.get_projection_mat() * input_handler.get_view_mat();
        // renderer.draw(camera_view);

        view = input_handler.get_projection_mat() * input_handler.get_view_mat();
        shader.use();
        shader.set_u_model(glm::identity<glm::mat4>());
        shader.set_u_transform(view);

        // glBindVertexArray(mesh.vao);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        // glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr);

        for (auto&& meshinfo : meshes) {
            glBindVertexArray(meshinfo.mesh.vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshinfo.mesh.ebo);

            auto pos = meshinfo.position;
            shader.set_u_model(glm::translate(glm::identity<glm::mat4>(), 
            { Chunk::Width * pos.x, Chunk::Height * pos.y, Chunk::Width * pos.z }));

            glDrawElements(GL_TRIANGLES, meshinfo.mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
        }

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
