#include <glbinding/gl46core/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
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
using glm::vec4;

using Face = std::array<vec3, 3>;
constexpr std::array boid {
    Face { vec3(0, .5, 0), vec3(.5, -.5, -.5), vec3(-.5, -.5, -.5) },
    Face { vec3(-.5, -.5, -.5), vec3(0, -.5, .5), vec3(0, .5, 0) },
    Face { vec3(0, -.5, .5), vec3(-.5, -.5, -.5), vec3(.5, -.5, -.5) },
    Face { vec3(.5, -.5, -.5), vec3(0, .5, 0), vec3(0, -.5, .5) },
};

constexpr std::array positions {
    vec4 { 0, 0, 0, 0 },
    vec4 { 0, 0, 0, 0 },
    vec4 { 0, 0, 0, 0 },
};
constexpr std::array velocities {
    vec4 { -.01, 0, 0, 0 },
    vec4 { .01, 0, 0, 0 },
    vec4 { 0, .01, 0, 0 },
};
constexpr std::array iColors {
    vec3 { 1, 0, 0 },
    vec3 { 0, 1, 0 },
    vec3 { 0, 0, 1 },
};

constexpr vec4 scene_size { 10, 10, 10, 0 };
constexpr std::array scene_walls {
    vec4 { scene_size.x / 2.f, 0, 0, 1 },
    vec4 { -scene_size.x / 2.f, 0, 0, 1 },
    vec4 { 0, scene_size.y / 2.f, 0, 1 },
    vec4 { 0, -scene_size.y / 2.f, 0, 1 },
    vec4 { 0, 0, scene_size.z / 2.f, 1 },
    vec4 { 0, 0, -scene_size.z / 2.f, 15 },
};

constexpr std::array boids_attribute_formats {
    vertex_attrib_format { 0, 0, 1, decltype(iColors)::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
    vertex_attrib_format { 1, 1, 0, decltype(boid)::value_type::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
};
const auto& [format_color, format_boid] = boids_attribute_formats;

int main()
{
    using namespace gl;
    try {
        const auto& gl_window = opengl::instance();

        const auto triangle = shader_program {
            gl_window,
            {
                shader_builder { "shaders/shader.vert.spv", GL_VERTEX_SHADER },
                shader_builder { "shaders/shader.frag.spv", GL_FRAGMENT_SHADER },
            }
        };
        const auto move_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/move.comp.spv", gl::GL_COMPUTE_SHADER },
            }
        };
        const auto debug_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/debug.vert.spv", GL_VERTEX_SHADER },
                shader_builder { "shaders/debug.frag.spv", GL_FRAGMENT_SHADER },
            }
        };

        const auto vao_boids = vertex_array { gl_window };
        vao_boids.format_attrib(std::span { boids_attribute_formats });

        const auto vao_walls = vertex_array { gl_window };

        const auto buf_colors = vertex_buffer { gl_window, std::span { iColors } };
        glVertexArrayVertexBuffer(vao_boids, format_color.binding_id, buf_colors, 0, sizeof(decltype(buf_colors)::buffer_t));

        const auto buf_boid = vertex_buffer { gl_window, std::span { &boid[0][0], boid.size() * std::tuple_size<decltype(boid)::value_type>::value } };
        glVertexArrayVertexBuffer(vao_boids, format_boid.binding_id, buf_boid, 0, sizeof(decltype(buf_boid)::buffer_t));

        const auto buf_positions = vertex_buffer { gl_window, std::span { positions } };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_positions);

        const auto buf_velocities = vertex_buffer { gl_window, std::span { velocities } };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_velocities);

        const auto buf_camera = vertex_buffer { gl_window, std::span { (glm::mat4*)nullptr, 2 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_time = vertex_buffer { gl_window, std::span { (float*)nullptr, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_camera);

        const auto buf_walls = vertex_buffer { gl_window, std::span(scene_walls) };
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, buf_walls);

        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        gl_window.render_loop([&] {
            const auto now = chr::duration<float> { chr::steady_clock::now().time_since_epoch() }.count();
            glNamedBufferSubData(buf_time, 0, buf_time.buffer_t_sizeof(), &now);

            using view_t = decltype(buf_camera)::buffer_t;
            // const vec3 view_point = glm::rotate(glm::mat4 { 1 }, 4 * std::sinf(.5f * now), vec3 { 0.f, 1.f, 0.f })
            //     * glm::vec4 { 0.f, 0.f, 5.f, 1.f };
            const auto next_view = glm::lookAt(vec3(0, 3, 10), glm::vec3 { 0.f, 0.f, 0.f }, glm::vec3 { 0.f, 1.f, 0.f });
            glNamedBufferSubData(buf_camera, 0, sizeof(view_t), &next_view[0][0]);

            using proj_t = decltype(buf_camera)::buffer_t;
            const auto next_persp = glm::perspective(45.f, gl_window.window_aspect(), .1f, 20.f);
            glNamedBufferSubData(buf_camera, sizeof(proj_t), sizeof(proj_t), &next_persp[0][0]);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(move_prog);
            glDispatchCompute(buf_positions.size(), 1, 1);

            glUseProgram(triangle);
            glBindVertexArray(vao_boids);
            glDrawArraysInstanced(GL_TRIANGLES, 0, buf_boid.size(), buf_positions.size());

            glUseProgram(debug_prog);
            glBindVertexArray(vao_walls);
            glDrawArrays(GL_LINE_LOOP, 0, buf_walls.size());
        });
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
