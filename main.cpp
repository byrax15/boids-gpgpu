#define GLFW_INCLUDE_NONE
#include <glbinding/gl46core/gl.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <iostream>
#include <numbers>
#include <print>
#include <ranges>
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
using position_t = glm::vec4;
using velocity_t = glm::vec4;

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

struct simulation {
    bool pause = false;
    bool iconified = false;
    float deltaTime = 0;
    std::vector<position_t> positions;
    std::vector<velocity_t> velocities;
};

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
};

int main()
{
    using namespace gl;
    try {
        const auto& gl_window = opengl::instance();
        simulation sim;
        camera cam;
        std::tuple user_pointers { &gl_window, &sim, &cam };
        glfwSetWindowUserPointer(*gl_window, &user_pointers);
        glfwSetKeyCallback(*gl_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            const auto& [glwin, sim, cam] = *reinterpret_cast<decltype(&user_pointers)>(glfwGetWindowUserPointer(window));
            if (action == GLFW_PRESS) {
                switch (key) {
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
                case GLFW_KEY_P:
                    sim->pause = !sim->pause;
                    break;
                }
            }
        });
        glfwSetFramebufferSizeCallback(*gl_window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });
        glfwSetWindowIconifyCallback(*gl_window, [](GLFWwindow* window, int iconified) {
            const auto& [glwin, sim, cam] = *reinterpret_cast<decltype(&user_pointers)>(glfwGetWindowUserPointer(window));
            sim->iconified = iconified;
        });
        glfwSetWindowSize(*gl_window, 1920, 1080);

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
                shader_builder { "shaders/debug_color.frag.spv", GL_FRAGMENT_SHADER, { 0, 255, 255 } },
            }
        };
        const auto debug_walls_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/debug_wall_corners.vert.spv", GL_VERTEX_SHADER },
                shader_builder { "shaders/debug_color.frag.spv", GL_FRAGMENT_SHADER, { 255, 255, 0 } },
            }
        };

        const auto vao_boids = vertex_array { gl_window };
        vao_boids.format_attrib(std::span { boids_attribute_formats });

        const auto vao_no_attributes = vertex_array { gl_window };

        const auto buf_colors = vertex_buffer {
            gl_window, nBoids, [] { return glm::rgbColor(glm::linearRand(vec3(0, 1, 1), vec3(360, 1, 1))); }
        };
        glVertexArrayVertexBuffer(vao_boids, format_color.binding_id, buf_colors, 0, sizeof(decltype(buf_colors)::buffer_t));

        const auto buf_boid = vertex_buffer { gl_window, std::span { &model_boid[0][0], model_boid.size() * std::tuple_size<Face>::value } };
        glVertexArrayVertexBuffer(vao_boids, format_boid.binding_id, buf_boid, 0, sizeof(decltype(buf_boid)::buffer_t));

        const auto buf_positions = vertex_buffer {
            gl_window, nBoids, [] { return glm::linearRand(vec4 { -scene_size, 0 }, vec4 { scene_size, 0 }); }
        };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_positions);

        const auto buf_velocities = vertex_buffer {
            gl_window, nBoids, [] { return glm::normalize(glm::linearRand(vec4 { -1, -1, -1, 0 }, vec4 { 1, 1, 1, 0 })); }
        };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_velocities);

        const auto buf_camera = vertex_buffer { gl_window, std::span { (glm::mat4*)nullptr, 2 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_time = vertex_buffer { gl_window, std::span { (float*)nullptr, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_time);

        const auto buf_scene_size = vertex_buffer { gl_window, std::span { &scene_size, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, buf_scene_size);

        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glPointSize(10.f);
        glLineWidth(5.f);

        float frame_time {}, last_time {};
        while (!glfwWindowShouldClose(*gl_window)) {
            glfwSwapBuffers(*gl_window);
            // glfwMakeContextCurrent(*gl_window);
            glfwPollEvents();
            if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_A)) {
                cam.azimuth -= cam.speed * sim.deltaTime;
            }
            if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_D)) {
                cam.azimuth += cam.speed * sim.deltaTime;
            }
            if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_S)) {
                cam.polar = std::clamp(cam.polar - cam.speed * sim.deltaTime, -.49f * glm::pi<float>(), .49f * glm::pi<float>());
            }
            if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_W)) {
                cam.polar = std::clamp(cam.polar + cam.speed * sim.deltaTime, -.49f * glm::pi<float>(), .49f * glm::pi<float>());
            }

            // deltaTime calc
            frame_time = glfwGetTime();
            sim.deltaTime = frame_time - last_time;
            last_time = frame_time;
            // deltaTime upload
            {
                const auto buffer = buf_time.map_buffer();
                buffer[0] = sim.deltaTime;
            }

            if (!sim.pause) {
                //{
                //    const auto mapped_pos = buf_positions.map_buffer();
                //    memcpy(mapped_pos.data(), sim.positions.data(), mapped_pos.size_bytes());
                //    const auto mapped_vel = buf_velocities.map_buffer();
                //    memcpy(mapped_vel.data(), sim.velocities.data(), mapped_vel.size_bytes());
                //}
                glUseProgram(move_prog);
                glDispatchCompute(buf_positions.size(), 1, 1);
                // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                //{
                //     const auto mapped_pos = buf_positions.map_buffer();
                //     memcpy(sim.positions.data(), mapped_pos.data(), mapped_pos.size_bytes());
                //     const auto mapped_vel = buf_velocities.map_buffer();
                //     memcpy(sim.velocities.data(), mapped_vel.data(), mapped_vel.size_bytes());
                // }
            }

            // stop rendering if minified
            if (sim.iconified)
                continue;

            // camera uniforms
            {
                const auto buffer = buf_camera.map_buffer();
                buffer[0] = cam.view();
                buffer[1] = glm::perspective(45.f, gl_window.window_aspect(), 1e-2f, 1e2f);
            }

            // draw boid models
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(boid_prog);
            glBindVertexArray(vao_boids);
            glDrawArraysInstanced(GL_TRIANGLES, 0, buf_boid.size(), buf_positions.size());

            // debug overlay
#ifndef NDEBUG
            glBindVertexArray(vao_no_attributes);
            glUseProgram(debug_velocities_prog);
            glDrawArraysInstanced(GL_LINES, 0, 2, buf_positions.size());

            glBindVertexArray(vao_no_attributes);
            glUseProgram(debug_walls_prog);
            glDrawArrays(GL_POINTS, 0, 8);
#endif

            // IMGUI code
            {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                static float last_update = 0;
                static std::array<float, 10> fps_samples;
                static size_t fps_iter = 0;
                if (frame_time - last_update >= 1.f) {
                    last_update = frame_time;
                    fps_samples[fps_iter] = 1.f / sim.deltaTime;
                    fps_iter = (fps_iter + 1) % fps_samples.size();
                }
                ImPlot::SetNextAxesToFit();
                if (ImGui::Begin("Debug View") && ImPlot::BeginPlot("FPS", { 200, 100 })) {
                    ImPlot::PlotBars("## fps", fps_samples.data(), fps_samples.size());
                    ImPlot::EndPlot();
//                    std::print(R"(
//p:{}
//v:{})",
//                        glm::to_string(sim.positions[0]), glm::to_string(sim.velocities[0]));
                }
                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
        }
    } catch (std::exception const& e) {
        std::cout << e.what() << "\n";
    }
}
