#ifndef RL_INPUT_HANDLER_HPP
#define RL_INPUT_HANDLER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define ON_KEY(key, action_block) { if (glfwGetKey(window, GLFW_KEY_##key) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_##key) == GLFW_REPEAT) action_block; }

class WindowInputHandler {
public:
    glm::mat4 get_view_mat() const;

    glm::mat4 get_projection_mat() const;

    void bind(GLFWwindow* p_window);

    void unbind();

    // Handle input controls that may be held down for any duration.
    // GLFW key repeat events do not register immediately, so a 
    // key listener that runs every frame is necessary for fast
    // repeat controls (like for navigation).
    void handle_framewise_key_input() {
        input_handler_impl(window);
    }

public:
    using Self = WindowInputHandler;
    #define CALL_IF_HANDLER_VALID(cb, ...) {auto* handler = reinterpret_cast<Self*>(glfwGetWindowUserPointer(window)); if (handler) handler->cb(__VA_ARGS__);}

    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_mouse_callback(GLFWwindow* window, double x_pos, double y_pos);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    void framebuffer_size_callback_impl(GLFWwindow* window, int width, int height);
    void mouse_callback_impl(GLFWwindow* window, double x_pos, double y_pos);
    void key_callback_impl(GLFWwindow* window, int key, int scancode, int action, int mods);
    void scroll_callback_impl(GLFWwindow* window, int xoffset, int yoffset);
    
    void input_handler_impl(GLFWwindow* window);

    void update_projection_cache();
    void update_view_cache();

    glm::vec3 camera_front = { 0.0f, 0.0f, -1.0f };
    glm::vec3 camera_pos = { 0.0f, 0.0f, 3.0f };
    glm::vec3 camera_up = { 0.0f, 1.0f, 0.0f };

    float camera_speed = 0.5f;
    float sensitivity = 0.1f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    float last_x = 0.0f;
    float last_y = 0.0f;

    float scroll_speedmult = 1.0f;
    float scroll_speedmult_min = 1.0f;
    float scroll_speedmult_max = 100.0f;

    float fov = glm::radians(60.0f);
    float aspect = 1.0f;
    float near = 0.1f;
    float far = 1000.0f;

    // There is currently no way to override the behavior of the projection algorithm
    // except through editing the fov, aspect, near, far parameters. I don't think
    // I'll ever add such functionality.
    glm::mat4 projection_cache = glm::perspective(fov, aspect, near, far);
    glm::mat4 view_cache = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);

    GLFWwindow* window = nullptr;
};

#endif 