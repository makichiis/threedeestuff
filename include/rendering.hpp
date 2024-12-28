#ifndef RL_RENDERING_HPP
#define RL_RENDERING_HPP

#include <glm/glm.hpp>

#include <glad/gl.h>

namespace Rendering {
    class ChunkShader {
    public:
        /**
         * @biref Singleton constructor for chunk shader. 
         */
        static auto get_chunk_shader() -> ChunkShader;

        /**
         * Binds this shader to the OpenGL draw context.
         */
        void use();

        void set_u_transform(glm::mat4 transform);

        void set_u_model(glm::mat4 model);

        auto program_id() const -> GLuint;
    private:
        ChunkShader() = default;

        GLuint m_program_id;
        GLuint u_transform_loc = 0;
        GLuint u_model_loc = 0;
    };

    
}

#endif 