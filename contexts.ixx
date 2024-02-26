//
// Created by byrax on 2024-02-24.
//
module;
#include <glbinding/gl46core/gl.h>
#include <glbinding/glbinding.h>

#include <GLFW/glfw3.h>

#include <format>
#include <stdexcept>
export module contexts;

export struct opengl {
private:
    opengl();

public:
    opengl(opengl const&) = delete;

    opengl& operator=(opengl const&) = delete;

    ~opengl()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    [[nodiscard]]
    operator GLFWwindow*() const { return window; }

    [[nodiscard]] static auto const& instance()
    {
        static opengl instance;
        return instance;
    }

    void render_loop(auto const& callable) const
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            callable();
            glfwSwapBuffers(window);
        }
    }

private:
    int width = 640, height = 480;
    GLFWwindow* window;
};

inline opengl::opengl()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(width, height, "Boids", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glbinding::initialize(glfwGetProcAddress);

    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int, int) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(w, true);
            break;
        default:
            break;
        }
    });

    gl::glEnable(gl::GL_DEBUG_OUTPUT);
    gl::glDebugMessageCallback(
        [](gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity, gl::GLsizei, const gl::GLchar* message, const void*) {
            if (type != gl::GLenum::GL_DEBUG_TYPE_ERROR)
                return;
            throw std::runtime_error(std::format(R"(GL ERROR::
    source:     {:x}
    type:       {:x}
    id:         {:x}
    severity:   {:x}
    message:    {:s})",
                (uint32_t)source, (uint32_t)type, (uint32_t)id, (uint32_t)severity, message));
        },
        nullptr);
}
