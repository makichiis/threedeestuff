#include <rendering.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include <windows.h>

using namespace Rendering;

// TODO: Move this to a dedicated header/CU, and refactor to take error info like shader file name, etc.
void crash_with_error(std::string_view message) {
    MessageBox(nullptr, message.data(), "VRL Engine", MB_ICONERROR | MB_OK);
    std::exit(1);
}

auto get_file_contents(std::string path) -> std::string {
    std::ifstream ifs(path);
    return std::string{ std::istreambuf_iterator<char>(ifs), 
        std::istreambuf_iterator<char>() };
}

auto create_shader_unit_from_source(GLenum type, std::string_view source) -> GLuint {
    GLuint shader = glCreateShader(type);
    
    auto source_datap = source.data();
    glShaderSource(shader, 1, &source_datap, nullptr);
    
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int info_log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        
        std::string info_log;
        info_log.resize(info_log_length);

        glGetShaderInfoLog(shader, info_log.capacity(), nullptr, info_log.data());
        auto s = std::string{ "Failed to compile shader: " } + info_log;
        crash_with_error(s);
        __builtin_unreachable();
    }

    return shader;
}

template <class... Shaders>
auto create_program_from_shader_units(Shaders... shaders) -> GLuint {
    GLuint program = glCreateProgram();

    ([&]{ glAttachShader(program, shaders); }(), ...);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        int info_log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);

        std::string info_log;
        info_log.resize(info_log_length);

        glGetProgramInfoLog(program, info_log.capacity(), nullptr, info_log.data());
        auto s = std::string{ "Failed to link shader program: " } + info_log;
        crash_with_error(s);
        __builtin_unreachable();
    }

    return program;
}

template <class... Shaders>
void delete_shaders(Shaders... shaders) {
    ([&]{ glDeleteShader(shaders); }(), ...);
}

auto Rendering::create_chunk_shader() -> GLuint {
    std::string vertex_shader_src = get_file_contents("resources/shaders/chunk_vertex.glsl");
    std::string fragment_shader_src = get_file_contents("resources/shaders/chunk_fragment.glsl");

    GLuint vertex_shader = create_shader_unit_from_source(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = create_shader_unit_from_source(GL_FRAGMENT_SHADER, fragment_shader_src);

    GLuint program = create_program_from_shader_units(vertex_shader, fragment_shader);
    delete_shaders(vertex_shader, fragment_shader);

    return program;
}

ChunkShader ChunkShader::get_chunk_shader() {
    static ChunkShader chunk_shader;

    if (chunk_shader.m_program_id == 0) {
        std::cout << "Creating chunk shader...\n";
        chunk_shader.m_program_id = create_chunk_shader();

        chunk_shader.u_transform_loc = glGetUniformLocation(chunk_shader.program_id(), "u_transform");
        chunk_shader.u_model_loc = glGetUniformLocation(chunk_shader.program_id(), "u_model");
    }

    return chunk_shader;
}

void ChunkShader::use() {
    glUseProgram(m_program_id);
}

void ChunkShader::set_u_transform(glm::mat4 m) {
    use();
    glUniformMatrix4fv(u_transform_loc, 1, GL_FALSE, glm::value_ptr(m));
}

void ChunkShader::set_u_model(glm::mat4 m) {
    use();
    glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, glm::value_ptr(m));
}

GLuint ChunkShader::program_id() const {
    return m_program_id;
}