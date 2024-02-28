#include <glbinding/gl46core/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <stdexcept>

import contexts;
import shader;
import vertex_buffer;
import vertex_array;

constexpr std::array colors {
    glm::vec3 { 1, 0, 0 },
    glm::vec3 { 0, 1, 0 },
    glm::vec3 { 0, 0, 1 },
};

constexpr std::array positions {
    glm::vec2 { 0, 0 },
    glm::vec2 { -.5, .6 },
    glm::vec2 { .4, -.7 },
};

int main()
{
    using namespace gl;
    try {
        const auto& gl_window = opengl::instance();

        const auto triangle = shader_program {
            gl_window,
            "shaders/shader.vert.spv",
            "shaders/shader.frag.spv"
        };
        glUseProgram(triangle);

        const auto vao = vertex_array { gl_window };
        glBindVertexArray(vao);

        const auto buf_colors = vertex_buffer { gl_window, std::span { colors } };
        buf_colors.bind_attribute(0, 0, 0);

        const auto buf_positions = vertex_buffer { gl_window, std::span { positions } };
        buf_positions.bind_attribute(1, 1, 1);

        const std::array camera {
            glm::lookAt(glm::vec3 { 0.f, 0.f, 3.f }, glm::vec3 { 0.f, 0.f, 0.f }, glm::vec3 { 0.f, 1.f, 0.f }),
            glm::perspective(glm::radians(45.f), gl_window.window_aspect(), .1f, 100.f)
        };
        const auto buf_camera = vertex_buffer {
            gl_window, std::span { camera }
        };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_mask_color = vertex_buffer { gl_window, glm::vec3 { 1, 0, 0 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_mask_color);

        glClearColor(0, 0, 0, 0);
        // glEnable(GL_DEPTH_TEST);
        gl_window.render_loop([&] {
            const auto next_persp = glm::perspective(45.f, gl_window.window_aspect(), .1f, 100.f);
            using proj_t = decltype(buf_camera)::buffer_t;
            glNamedBufferSubData(buf_camera, sizeof(proj_t), sizeof(proj_t), &next_persp[0][0]);

            glDrawArraysInstanced(GL_TRIANGLES, 0, 3, static_cast<GLsizei>(positions.size()));
        });
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
