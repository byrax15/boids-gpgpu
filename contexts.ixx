//
// Created by byrax on 2024-02-24.
//
module;
#define GLFW_INCLUDE_NONE
#include <glbinding/gl46core/gl.h>
#include <glbinding/glbinding.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <format>
#include <print>
#include <stdexcept>
export module contexts;

export struct opengl {
private:
    opengl();

public:
    ~opengl()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    opengl(opengl const&) = delete;
    opengl& operator=(opengl const&) = delete;

    [[nodiscard]] static auto const& instance()
    {
        static opengl instance;
        return instance;
    }

    auto window_aspect() const
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        return static_cast<float>(w) / static_cast<float>(h);
    }

    auto operator*() const { return window; }

private:
    GLFWwindow* window;
};

opengl::opengl()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(640, 480, "Boids GPGPU", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glbinding::initialize(glfwGetProcAddress);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

#ifndef NDEBUG
    gl::glEnable(gl::GL_DEBUG_OUTPUT);
    gl::glDebugMessageCallback(
        [](gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity, gl::GLsizei, const gl::GLchar* message, const void*) {
            if (type != gl::GLenum::GL_DEBUG_TYPE_ERROR)
                return;
            std::print(R"(GL ERROR::
    source:     0x{:x}
    type:       0x{:x}
    id:         0x{:x}
    severity:   0x{:x}
    message:    {:s})",
                (uint32_t)source, (uint32_t)type, (uint32_t)id, (uint32_t)severity, message);
            std::terminate();
        },
        nullptr);
#endif
}
