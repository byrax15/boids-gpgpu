#include <glbinding/gl46core/gl.h>

#define GLFW_INCLUDE_NONE
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
#include <exception>
#include <numeric>
#include <print>
#include <ranges>
#include <span>
#include <vector>

import contexts;
import shader;
import vertex_array;
import vertex_buffer;

using glm::vec3;
using glm::vec4;
using color_t = glm::vec4;
using position_t = glm::vec4;
using velocity_t = glm::vec4;
using normal_t = glm::vec4;

struct Face {
    using positions_t = std::array<position_t, 3>;
    using normals_t = std::array<normal_t, 3>;

    static constexpr normals_t computeNormals(positions_t const& positions)
    {
        const std::array<vec3, 2> barycentric {
            positions[2] - positions[0],
            positions[1] - positions[0],
        };
        const auto normal = normal_t(glm::cross(barycentric[0], barycentric[1]), 0);
        return { normal, normal, normal };
    }
};
struct Light {
    vec4 position, color;
} light(vec4(0.f, 10.f, 0.f, 1.f), vec4(1.f));

constexpr std::array boid_model_positions {
    Face::positions_t { position_t(0, .5, 0, 1), position_t(.5, -.5, -.5, 1), position_t(-.5, -.5, -.5, 1) },
    Face::positions_t { position_t(-.5, -.5, -.5, 1), position_t(0, -.5, .5, 1), position_t(0, .5, 0, 1) },
    Face::positions_t { position_t(0, -.5, .5, 1), position_t(-.5, -.5, -.5, 1), position_t(.5, -.5, -.5, 1) },
    Face::positions_t { position_t(.5, -.5, -.5, 1), position_t(0, .5, 0, 1), position_t(0, -.5, .5, 1) },
};
constexpr auto boid_model_normals = std::invoke([&] {
    constexpr auto size = std::tuple_size<decltype(boid_model_positions)> {};
    std::array<Face::normals_t, size> face_normal;
    for (const auto& [p, n] : std::views::zip(boid_model_positions, face_normal)) {
        n = Face::computeNormals(p);
    }
    return face_normal;
});

template <typename T>
constexpr size_t md_size()
{
    using array_t = std::remove_cvref_t<T>;
    return std::tuple_size<array_t> {}
    * std::tuple_size<typename array_t::value_type> {};
}

constexpr std::array boids_attribute_formats {
    vertex_attrib_format { 0, 0, 0, position_t::length(), 0, gl::GL_FLOAT, gl::GL_FALSE },
    vertex_attrib_format { 1, 1, 0, normal_t::length(), 0, gl::GL_FLOAT, gl::GL_TRUE },
};
const auto& [format_model_position, format_model_normal] = boids_attribute_formats;

struct simulation {
    bool pause = false, debug_vel = false;
    float deltaTime = 0;
    vec3 scene_size = 4.f * vec3 { 5, 3, 5 };
    size_t nBoids = 200;
    std::array<float, 2> boid_sights { 5.f, 2.f };
    std::array<float, 4> boid_goal_strengths { .4f, .2f, 1.f, .4f };
} sim;

struct camera {
    float azimuth {}, polar {}, zoom = 2.f * *std::ranges::max_element(std::span((float*)&sim.scene_size, 3));
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

    auto incr_zoom(float delta)
    {
        zoom = std::max(zoom + delta, 0.f);
        return zoom;
    }

    auto pan_vert(float amplitude)
    {
        polar = std::clamp(polar + amplitude * speed * sim.deltaTime, -.49f * glm::pi<float>(), .49f * glm::pi<float>());
    }

    auto pan_hori(float amplitude)
    {
        azimuth -= amplitude * speed * sim.deltaTime;
    }

    auto view() const
    {
        return glm::lookAt(eye(), focus, vec3(0, 1, 0));
    }
} cam;

int main()
{
    using namespace gl;
    try {
        const auto& gl_window = opengl::instance();
        glfwSetWindowUserPointer(*gl_window, (void*)&gl_window);
        glfwSetKeyCallback(*gl_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            const auto& glwin = *reinterpret_cast<decltype(&gl_window)>(glfwGetWindowUserPointer(window));
            auto& io = ImGui::GetIO();
            if (io.WantCaptureKeyboard)
                return;

            if (action == GLFW_PRESS) {
                switch (key) {
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
                case GLFW_KEY_P:
                    sim.pause = !sim.pause;
                    break;
                case GLFW_KEY_O:
                    glwin.toggle_mouse_capture();
                    break;
                case GLFW_KEY_ENTER:
                    if (mods == GLFW_MOD_ALT) {
                        glwin.toggle_fullscreen();
                    }
                }
            }
        });
        glfwSetCursorPosCallback(*gl_window, [](GLFWwindow* window, double xpos, double ypos) {
            const auto& glwin = *reinterpret_cast<decltype(&gl_window)>(glfwGetWindowUserPointer(window));
            if (glwin.is_mouse_captured()) {
                static glm::vec2 last_pos { xpos, ypos };
                glm::vec2 new_pos { xpos, ypos };
                glm::ivec2 i_win_size;
                glfwGetWindowSize(window, &i_win_size.x, &i_win_size.y);
                const auto percent_change = (new_pos - last_pos) /*/ glm::vec2(i_win_size)*/;
                cam.pan_hori(percent_change.x);
                cam.pan_vert(-percent_change.y);
                last_pos = new_pos;
                return;
            }
            auto& io = ImGui::GetIO();
            io.AddMousePosEvent(xpos, ypos);
            if (io.WantCaptureMouse)
                return;
        });
        glfwSetScrollCallback(*gl_window, [](GLFWwindow* window, double xoffset, double yoffset) {
            const auto& glwin = *reinterpret_cast<decltype(&gl_window)>(glfwGetWindowUserPointer(window));
            if (glwin.is_mouse_captured()) {
                cam.incr_zoom(-yoffset);
                return;
            }
            auto& io = ImGui::GetIO();
            io.AddMouseWheelEvent(xoffset, yoffset);
            if (io.WantCaptureMouse)
                return;
        });
        glfwSetFramebufferSizeCallback(*gl_window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });
        glfwSetWindowIconifyCallback(*gl_window, [](GLFWwindow* window, int iconified) {
            const auto& glwin = *reinterpret_cast<opengl*>(glfwGetWindowUserPointer(window));
            glwin.set_iconified(iconified);
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
        const auto interactions_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/interactions.comp.spv", gl::GL_COMPUTE_SHADER },
            }
        };
        const auto debug_velocities_prog = shader_program {
            gl_window,
            {
                shader_builder { "shaders/debug_velocities.vert.spv", GL_VERTEX_SHADER },
                shader_builder { "shaders/debug_color.frag.spv", GL_FRAGMENT_SHADER, { 255, 255, 255 } },
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

        const auto buf_boid_positions = vertex_buffer { gl_window, std::span((position_t*)&boid_model_positions, md_size<decltype(boid_model_positions)>()) };
        glVertexArrayVertexBuffer(vao_boids, format_model_position.binding_id, buf_boid_positions, 0, buf_boid_positions.buffer_t_sizeof());

        const auto buf_boid_normals = vertex_buffer { gl_window, std::span((normal_t*)&boid_model_normals, md_size<decltype(boid_model_normals)>()) };
        glVertexArrayVertexBuffer(vao_boids, format_model_normal.binding_id, buf_boid_normals, 0, buf_boid_normals.buffer_t_sizeof());

        const auto buf_positions = vertex_buffer {
            gl_window, sim.nBoids, [] { return glm::linearRand(.5f * vec4 { -sim.scene_size, 0 }, .5f * vec4 { sim.scene_size, 0 }); }
        };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_positions);

        const auto buf_velocities = vertex_buffer {
            gl_window, sim.nBoids, [] { return (glm::linearRand(vec4 { -1, -1, -1, 0 }, vec4 { 1, 1, 1, 0 })); }
        };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_velocities);

        const auto buf_colors = vertex_buffer {
            gl_window, sim.nBoids, [] { return vec4(glm::rgbColor(glm::linearRand(vec3(0, 1, 1), vec3(360, 1, 1))), 1); }
        };
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_colors);

        const auto buf_camera = vertex_buffer { gl_window, std::span { (glm::mat4*)nullptr, 2 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf_camera);

        const auto buf_time = vertex_buffer { gl_window, std::span { (float*)nullptr, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buf_time);

        const auto buf_scene_size = vertex_buffer { gl_window, std::span { &sim.scene_size, 1 } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, buf_scene_size);

        const auto buf_boid_sights = vertex_buffer { gl_window, std::span { sim.boid_sights } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, buf_boid_sights);

        const auto buf_boid_goals = vertex_buffer { gl_window, std::span { sim.boid_goal_strengths } };
        glBindBufferBase(GL_UNIFORM_BUFFER, 4, buf_boid_goals);

        const auto buf_light = vertex_buffer { gl_window, std::span(&light, 1) };
        glBindBufferBase(GL_UNIFORM_BUFFER, 5, buf_light);

        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glPointSize(10.f);
        glLineWidth(5.f);

        float frame_time {}, last_time {};
        while (!glfwWindowShouldClose(*gl_window)) {
            glfwMakeContextCurrent(*gl_window);
            glfwSwapBuffers(*gl_window);
            glfwPollEvents();
            {
                if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_A)) {
                    cam.pan_hori(-1);
                }
                if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_D)) {
                    cam.pan_hori(1);
                }
                if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_S)) {
                    cam.pan_vert(-1);
                }
                if (GLFW_PRESS == glfwGetKey(*gl_window, GLFW_KEY_W)) {
                    cam.pan_vert(1);
                }
            }

            // deltaTime calc
            frame_time = glfwGetTime();
            sim.deltaTime = frame_time - last_time;
            last_time = frame_time;
            // upload sim params
            {
                buf_time.map_buffer()[0] = sim.deltaTime;
                std::ranges::copy(sim.boid_sights, buf_boid_sights.map_buffer().begin());
                std::ranges::copy(sim.boid_goal_strengths, buf_boid_goals.map_buffer().begin());
            }

            // physics
            if (!sim.pause) {
                glUseProgram(move_prog);
                glDispatchCompute(buf_positions.size(), 1, 1);
                // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                glUseProgram(interactions_prog);
                glDispatchCompute(buf_positions.size(), 1, 1);
            }

            // stop rendering if minified
            if (gl_window.is_iconified())
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
            glDrawArraysInstanced(GL_TRIANGLES, 0, buf_boid_positions.size(), buf_positions.size());

            // debug overlay
            if (sim.debug_vel) {
                glBindVertexArray(vao_no_attributes);
                glUseProgram(debug_velocities_prog);
                glDrawArraysInstanced(GL_LINES, 0, 2, buf_positions.size());
            }

            glBindVertexArray(vao_no_attributes);
            glUseProgram(debug_walls_prog);
            glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 2);

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
                    for (const auto& [l, r] : fps_samples | std::views::pairwise) {
                        l = r;
                    }
                    *fps_samples.rbegin() = 1.f / sim.deltaTime;
                }
                ImPlot::SetNextAxesToFit();
                if (ImGui::Begin("Monitor") && ImPlot::BeginPlot("FPS", { ImGui::GetColumnWidth(), 100 })) {
                    constexpr auto xs = std::invoke([] {
                        std::array<float, 10> out;
                        for (auto i = out.begin(); i != out.end(); ++i)
                            *i = -9 + std::distance(out.begin(), i);
                        return out;
                    });
                    ImPlot::PlotLine("## fps legend", xs.data(), fps_samples.data(), fps_samples.size());
                    ImPlot::EndPlot();
                }
                ImGui::End();

                if (ImGui::Begin("Controls")) {
                    ImGui::Text("Camera mov.:          WASD & Mouse");
                    ImGui::Text("Quit:                 ESC");
                    ImGui::Text("Pause physics:        P");
                    ImGui::Text("Toggle mouse capture: O");
                    ImGui::Text("Toggle fullscreen:    ALT+ENTER");
                }
                ImGui::End();

                if (ImGui::Begin("Simulation")) {
                    ImGui::DragFloat("Sight range", &sim.boid_sights[0], .1f, 0, 0, "%.3f", 0);
                    ImGui::DragFloat("Avoidance range", &sim.boid_sights[1], .1f, 0, sim.boid_sights[0], "%.3f", 0);
                    ImGui::DragFloat("Cohesion strength", &sim.boid_goal_strengths[0], .01f, 0, 0, "%.3f", 0);
                    ImGui::DragFloat("Alignment strength", &sim.boid_goal_strengths[1], .01f, 0, 0, "%.3f", 0);
                    ImGui::DragFloat("Avoidance strength", &sim.boid_goal_strengths[2], .01f, 0, 0, "%.3f", 0);
                    ImGui::DragFloat("Disturbance strength", &sim.boid_goal_strengths[3], .01f, 0, 0, "%.3f", 0);
                    ImGui::Checkbox("Debug velocity view", &sim.debug_vel);
                }
                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
        }
    } catch (std::exception const& e) {
        std::print("{}\n", e.what());
    }
}
