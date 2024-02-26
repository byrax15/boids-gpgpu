#include <glbinding/gl46core/gl.h>
#include <glm/glm.hpp>

#include <array>
#include <exception>
#include <iostream>
#include <span>

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

        const auto buf_colors = vertex_buffer {
            gl_window, std::span { colors }
        };
        glBindVertexBuffer(0, buf_colors, 0, sizeof(decltype(buf_colors)::buffer_t));
        glEnableVertexAttribArray(0);
        glVertexAttribFormat(0, decltype(buf_colors)::buffer_t::length(), GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 0);

        
        const auto buf_positions = vertex_buffer {
            gl_window, std::span { positions }
        };
        glBindVertexBuffer(1, buf_positions, 0, sizeof(decltype(buf_positions)::buffer_t));
        glEnableVertexAttribArray(1);
        glVertexAttribFormat(1, decltype(buf_positions)::buffer_t::length(), GL_FLOAT, GL_FALSE, 0);
        glVertexBindingDivisor(1, 1);
        glVertexAttribBinding(1, 1);

        glClearColor(0, 0, 0, 0);
        gl_window.render_loop([&] {
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArraysInstanced(GLenum::GL_TRIANGLES, 0, 3, static_cast<GLsizei>(positions.size()));
        });
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
 