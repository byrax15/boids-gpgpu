#include <glbinding/gl46core/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <stdexcept>
#include <type_traits>
import contexts;
import shader;
import vertex_buffer;
import vertex_array;

namespace chr = std::chrono;
using glm::vec3;

using FaceColor = std::array<vec3, 3>;
constexpr std::array colors {
    FaceColor { vec3 { 1, 0, 0 }, vec3 { 1, 0, 0 }, vec3 { 1, 0, 0 } },
    FaceColor { vec3 { 0, 1, 0 }, vec3 { 0, 1, 0 }, vec3 { 0, 1, 0 } },
    FaceColor { vec3 { 0, 0, 1 }, vec3 { 0, 0, 1 }, vec3 { 0, 0, 1 } },
    FaceColor { vec3 { 1, 0, 1 }, vec3 { 1, 0, 1 }, vec3 { 1, 0, 1 } },
};

using Face = std::array<vec3, 3>;
constexpr std::array boid {
    Face { vec3(0, .5, 0), vec3(.5, -.5, -.5), vec3(-.5, -.5, -.5) },
    Face { vec3(-.5, -.5, -.5), vec3(0, -.5, .5), vec3(0, .5, 0) },
    Face { vec3(0, -.5, .5), vec3(-.5, -.5, -.5), vec3(.5, -.5, -.5) },
    Face { vec3(.5, -.5, -.5), vec3(0, .5, 0), vec3(0, -.5, .5) },
};

constexpr std::array positions {
    vec3 { 0, 0, 0 },
    vec3 { -1, 0, 1 },
    vec3 { 2, 0, .5 },
};

constexpr std::array attribute_formats {
    vertex_attrib_format { 0, 0, 0, decltype(colors)::value_type::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
    vertex_attrib_format { 1, 1, 0, decltype(boid)::value_type::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
    vertex_attrib_format { 2, 2, 1, decltype(positions)::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
};
const auto& [format_color, format_boid, format_positions] = attribute_formats;

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

        const auto buf_colors = vertex_buffer { gl_window, std::span { &colors[0][0], colors.size() * std::tuple_size<decltype(colors)::value_type>::value } };
        const auto buf_boid = vertex_buffer { gl_window, std::span { &boid[0][0], boid.size() * std::tuple_size<decltype(boid)::value_type>::value } };
        const auto buf_positions = vertex_buffer { gl_window, std::span { positions } };

        const auto buf_camera = vertex_buffer { gl_window, std::span { (glm::mat4*)nullptr, 2 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_mask_color = vertex_buffer { gl_window, glm::vec3 { 0, 0, 0 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_mask_color);

        const auto vao = vertex_array { gl_window };
        vao.format_attrib(std::span { attribute_formats });
        glVertexArrayVertexBuffer(vao, format_color.binding_id, buf_colors, 0, sizeof(decltype(buf_colors)::buffer_t));
        glVertexArrayVertexBuffer(vao, format_boid.binding_id, buf_boid, 0, sizeof(decltype(buf_boid)::buffer_t));
        glVertexArrayVertexBuffer(vao, format_positions.binding_id, buf_positions, 0, sizeof(decltype(buf_positions)::buffer_t));
        glBindVertexArray(vao);

        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        gl_window.render_loop([&] {
            const auto now = chr::duration<float> { chr::steady_clock::now().time_since_epoch() }.count();

            using view_t = decltype(buf_camera)::buffer_t;
            const vec3 view_point = glm::rotate(glm::mat4 { 1 }, 4 * std::sinf(.5f * now), vec3 { 0.f, 1.f, 0.f })
                * glm::vec4 { 0.f, 0.f, 5.f, 1.f };
            const auto next_view = glm::lookAt(view_point, glm::vec3 { 0.f, 0.f, 0.f }, glm::vec3 { 0.f, 1.f, 0.f });
            glNamedBufferSubData(buf_camera, 0, sizeof(view_t), &next_view[0][0]);

            using proj_t = decltype(buf_camera)::buffer_t;
            const auto next_persp = glm::perspective(45.f, gl_window.window_aspect(), .1f, 20.f);
            glNamedBufferSubData(buf_camera, sizeof(proj_t), sizeof(proj_t), &next_persp[0][0]);

            using color_t = decltype(buf_mask_color)::buffer_t;
            const auto next_mask = color_t { std::cos(now) + 1.f, 0, 0 };
            glNamedBufferSubData(buf_mask_color, 0, sizeof(color_t), &next_mask);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArraysInstanced(GL_TRIANGLES, 0,
                boid.size() * std::tuple_size<decltype(boid)::value_type>::value,
                static_cast<GLsizei>(positions.size()));
        });
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
