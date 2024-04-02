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

#include <implot.h>

#include <format>
#include <optional>
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
        ImPlot::DestroyContext();
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

    auto toggle_fullscreen() const
    {
        if (fullscreen) {
            glfwSetWindowMonitor(window, nullptr, x, y, w, h, GLFW_DONT_CARE);
            glfwSwapInterval(1);
        } else {
            glfwGetWindowPos(window, &x, &y);
            glfwGetWindowSize(window, &w, &h);
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), x, y, w, h, GLFW_DONT_CARE);
            glfwSwapInterval(1);
        }
        fullscreen = !fullscreen;
    }

    auto toggle_mouse_capture() const
    {
        set_mouse_capture(!mouse_captured);
        return mouse_captured;
    }

    auto is_mouse_captured() const { return mouse_captured; }
    auto is_iconified() const { return iconified; }
    void set_iconified(bool iconified) const { iconified = iconified; }

private:
    void set_mouse_capture(bool capture) const
    {
        if (capture) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
        mouse_captured = capture;
    }

    GLFWwindow* window;
    mutable bool mouse_captured, fullscreen, iconified;
    mutable int x, y, w, h; // previous
};

opengl::opengl()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
    window = glfwCreateWindow(640, 480, "Boids GPGPU", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glbinding::initialize(glfwGetProcAddress);
    set_mouse_capture(false);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    //auto& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
