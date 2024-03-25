#define GLFW_INCLUDE_NONE
#include <glbinding/gl46core/gl.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <iostream>
#include <numbers>
#include <print>
#include <span>
#include <thread>
#include <vector>

import contexts;
import shader;
import vertex_array;
import vertex_buffer;

namespace chr = std::chrono;
using glm::vec3;
using glm::vec4;
using i_color_t = glm::vec4;

using Face = std::array<vec3, 3>;
constexpr std::array model_boid {
    Face { vec3(0, .5, 0), vec3(.5, -.5, -.5), vec3(-.5, -.5, -.5) },
    Face { vec3(-.5, -.5, -.5), vec3(0, -.5, .5), vec3(0, .5, 0) },
    Face { vec3(0, -.5, .5), vec3(-.5, -.5, -.5), vec3(.5, -.5, -.5) },
    Face { vec3(.5, -.5, -.5), vec3(0, .5, 0), vec3(0, -.5, .5) },
};

constexpr vec3 scene_size { 5, 5, 5 };
constexpr size_t nBoids = 20;
constexpr std::array boids_attribute_formats {
    vertex_attrib_format { 0, 0, 1, i_color_t::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
    vertex_attrib_format { 1, 1, 0, Face::value_type::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
};
const auto& [format_color, format_boid] = boids_attribute_formats;

struct sim_state {
    bool pause = false;
    float deltaTime = 0;
} sim;

struct camera {
    float azimuth {}, polar {}, zoom = 20.f;
    float speed = 1.f;

    glm::vec3 focus {};

    auto eye() const
    {
        return focus + zoom * glm::vec3 {
            cosf(polar) * cosf(azimuth),
            sinf(polar),
            cosf(polar) * sinf(azimuth),
        };
    }

    auto view() const
    {
        return glm::lookAt(eye(), focus, vec3(0, 1, 0));
    }

} camera;

int main()
{
    using namespace gl;
    try {
        const auto& gl_window = opengl::instance();
        glfwSetKeyCallback(*gl_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS) {
                switch (key) {
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
                case GLFW_KEY_P:
                    sim.pause = !sim.pause;
                    break;
                }
            }
            switch (key) {
            case GLFW_KEY_LEFT:
                camera.azimuth -= camera.speed * sim.deltaTime;
                break;
            case GLFW_KEY_RIGHT:
                camera.azimuth += camera.speed * sim.deltaTime;
                break;
            case GLFW_KEY_DOWN:
                camera.polar = std::clamp(camera.polar - camera.speed * sim.deltaTime, -.49f * glm::pi<float>(), .49f * glm::pi<float>());
                break;
            case GLFW_KEY_UP:
                camera.polar = std::clamp(camera.polar + camera.speed * sim.deltaTime, -.49f * glm::pi<float>(), .49f * glm::pi<float>());
                break;
            }
        });
        glfwSetFramebufferSizeCallback(*gl_window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });

        const auto boid_prog = shader_program {
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
        const auto debug_velocities_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/debug_velocities.vert.spv", GL_VERTEX_SHADER },
                shader_builder { "shaders/debug_color.frag.spv", GL_FRAGMENT_SHADER, { 255, 0, 255 } },
            }
        };

        const auto vao_boids = vertex_array { gl_window };
        vao_boids.format_attrib(std::span { boids_attribute_formats });

        const auto vao_no_attributes = vertex_array { gl_window };

        const auto buf_colors = vertex_buffer { gl_window, std::span { (glm::vec3*)nullptr, nBoids } };
        std::ranges::for_each(buf_colors.map_buffer(), [](auto& c) {
            c = glm::rgbColor(glm::linearRand(vec3 { 0, 1, 1 }, vec3 { 360, 1, 1 }));
        });
        glVertexArrayVertexBuffer(vao_boids, format_color.binding_id, buf_colors, 0, sizeof(decltype(buf_colors)::buffer_t));

        const auto buf_boid = vertex_buffer { gl_window, std::span { &model_boid[0][0], model_boid.size() * std::tuple_size<Face>::value } };
        glVertexArrayVertexBuffer(vao_boids, format_boid.binding_id, buf_boid, 0, sizeof(decltype(buf_boid)::buffer_t));

        const auto buf_positions = vertex_buffer { gl_window, std::span { (glm::vec4*)nullptr, nBoids } };
        std::ranges::for_each(buf_positions.map_buffer(), [](auto& p) {
            p = glm::linearRand(vec4 { scene_size, 1 }, vec4 { scene_size, 1 });
        });
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_positions);

        const auto buf_velocities = vertex_buffer { gl_window, std::span { (glm::vec4*)nullptr, nBoids } };
        std::ranges::for_each(buf_velocities.map_buffer(), [](auto& v) {
            v = glm::linearRand(.5f * vec4 { -1, -1, -1, 0 }, .1f * vec4 { 1, 1, 1, 0 });
            // v = {};
        });
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_velocities);

        const auto buf_camera = vertex_buffer { gl_window, std::span { (glm::mat4*)nullptr, 2 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_time = vertex_buffer { gl_window, std::span { (float*)nullptr, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_camera);

        const auto buf_scene_size = vertex_buffer { gl_window, std::span { &scene_size, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, buf_scene_size);

        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glPointSize(10.f);
        glLineWidth(5.f);

        float frame_time{}, last_time {};
        while (!glfwWindowShouldClose(*gl_window)) {
            frame_time = glfwGetTime();
            sim.deltaTime = frame_time - last_time;
            last_time = frame_time;
            glNamedBufferSubData(buf_time, 0, buf_time.size_bytes(), &sim.deltaTime);

            const auto view = camera.view();
            glNamedBufferSubData(buf_camera, 0, sizeof(view), &view[0][0]);

            using proj_t = decltype(buf_camera)::buffer_t;
            const auto next_persp = glm::perspective(45.f, gl_window.window_aspect(), 1e-2f, 1e2f);
            glNamedBufferSubData(buf_camera, sizeof(proj_t), sizeof(proj_t), &next_persp[0][0]);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (!sim.pause) {
                glUseProgram(move_prog);
                glDispatchCompute(buf_positions.size(), 1, 1);
            }

            glUseProgram(boid_prog);
            glBindVertexArray(vao_boids);
            glDrawArraysInstanced(GL_TRIANGLES, 0, buf_boid.size(), buf_positions.size());

            glBindVertexArray(vao_no_attributes);

            glUseProgram(debug_velocities_prog);
            glDrawArraysInstanced(GL_LINES, 0, 2, buf_positions.size());

            glfwPollEvents();
            //{
            //    ImGui_ImplOpenGL3_NewFrame();
            //    ImGui_ImplGlfw_NewFrame();
            //    ImGui::NewFrame();

            //    //ImGui::Text("FPS %f", 1.f / deltaTime);

            //    ImGui::Render();
            //    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            //}
            glfwSwapBuffers(*gl_window);
        }
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
