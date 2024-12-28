#include <input_handler.hpp>

glm::mat4 WindowInputHandler::get_view_mat() const {
    return view_cache;
}

glm::mat4 WindowInputHandler::get_projection_mat() const {
    return projection_cache;
}

inline void fastSinCos(double x, double &sinOut, double &cosOut) {
    const double x2 = x * x;
    sinOut = x - (x2 * x) / 6.0 + (x2 * x2 * x) / 120.0;
    cosOut = 1.0 - x2 / 2.0 + (x2 * x2) / 24.0;      
}

void WindowInputHandler::bind(GLFWwindow* p_window) {
    unbind();

    window = p_window;
    glfwSetWindowUserPointer(window, reinterpret_cast<void*>(this));

    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetScrollCallback(window, glfw_scroll_callback);
}

void WindowInputHandler::unbind() {
    if (window && glfwGetWindowUserPointer(window) != nullptr) {
        glfwSetWindowUserPointer(window, nullptr);

        glfwSetFramebufferSizeCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetKeyCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);

        window = nullptr;
    }
}

void WindowInputHandler::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    CALL_IF_HANDLER_VALID(framebuffer_size_callback_impl, window, width, height);
}

void WindowInputHandler::glfw_mouse_callback(GLFWwindow* window, double x_pos, double y_pos) {
    CALL_IF_HANDLER_VALID(mouse_callback_impl, window, x_pos, y_pos);
}

void WindowInputHandler::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    CALL_IF_HANDLER_VALID(key_callback_impl, window, key, scancode, action, mods);
}

void WindowInputHandler::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    CALL_IF_HANDLER_VALID(scroll_callback_impl, window, xoffset, yoffset);
}

void WindowInputHandler::framebuffer_size_callback_impl(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    aspect = static_cast<float>(width) / static_cast<float>(height); // this kinda totally kills scalablity but i dont care for now

    update_projection_cache(); // this kinda totally kills scalablity but i dont care for now
}

void WindowInputHandler::mouse_callback_impl(GLFWwindow* window, double x_pos, double y_pos) {
    static bool first_mouse = true;
    if (first_mouse) {
        last_x = x_pos;
        last_y = y_pos;
        first_mouse = false;
    }

    float x_offset = x_pos - last_x;
    float y_offset = last_y - y_pos;
    last_x = x_pos;
    last_y = y_pos;

    x_offset *= sensitivity;
    y_offset *= sensitivity;

    yaw += x_offset;
    pitch += y_offset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }
    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera_front = glm::normalize(direction);

    update_view_cache(); // this kinda totally kills scalablity but i dont care for now
}

void WindowInputHandler::key_callback_impl(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    static auto polygon_mode = GL_FILL;
    if (key == GLFW_KEY_F && glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (polygon_mode == GL_FILL)
            polygon_mode = GL_LINE;
        else 
            polygon_mode = GL_FILL;
    
        glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
    }

    static bool cull_face = true;
    if (key == GLFW_KEY_C && glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        cull_face = !cull_face;
        
        if (cull_face) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }
}
#include <iostream>

#define clamp_high(x, m) ((x > m) ? m : x) 
#define clamp_low(x, m) ((x < m) ? m : x)
void WindowInputHandler::scroll_callback_impl(GLFWwindow* window, int xoffset, int yoffset) {
    std::cout << yoffset << '\n';
    if (yoffset > 0)
        scroll_speedmult = clamp_high(scroll_speedmult + yoffset, scroll_speedmult_max);
    else if (yoffset < 0)
        scroll_speedmult = clamp_low(scroll_speedmult + yoffset, scroll_speedmult_min);
}

void WindowInputHandler::input_handler_impl(GLFWwindow* window) {
    static float speed = 1.0f * scroll_speedmult;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed = 5.0f * scroll_speedmult;
    } else {
        speed = 1.0f * scroll_speedmult;
    }
    ON_KEY(W, (camera_pos += camera_speed * camera_front * speed));
    ON_KEY(S, (camera_pos -= camera_speed * camera_front * speed));
    ON_KEY(A, (camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed * speed));
    ON_KEY(D, (camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed * speed));

    update_view_cache(); // this kinda totally kills scalablity but i dont care for now
}

void WindowInputHandler::update_projection_cache() {
    projection_cache = glm::perspective(fov, aspect, near, 100000.0f);
}

void WindowInputHandler::update_view_cache() {
    view_cache = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
}