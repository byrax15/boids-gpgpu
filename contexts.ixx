//
// Created by byrax on 2024-02-24.
//
module;
#include <glbinding/gl46core/gl.h>
#include <glbinding/glbinding.h>

#include <SFML/Window.hpp>

#include <format>
#include <stdexcept>
export module contexts;

export struct opengl {
private:
    opengl();

public:
    opengl(opengl const&) = delete;

    opengl& operator=(opengl const&) = delete;

    [[nodiscard]] static auto const& instance()
    {
        static opengl instance;
        return instance;
    }

    auto window_aspect() const
    {
        const auto [w, h] = window.getSize();
        return static_cast<float>(w) / static_cast<float>(h);
    }

    auto operator->() const { return &window; }

private:
    mutable sf::Window window;
};

opengl::opengl()
    : window(sf::Window { sf::VideoMode(1920, 1080, 32), "Boids", sf::Style::Default, std::invoke([] {
                             using ctx_attrib = sf::ContextSettings::Attribute;
#ifdef NDEBUG
                             const auto flags = ctx_attrib::Core;
#else
                             const auto flags = ctx_attrib::Core | ctx_attrib::Debug;
#endif
                             return sf::ContextSettings(24, 8, 2, 4, 6, flags);
                         }) })
{
    window.setVerticalSyncEnabled(true);
    window.setActive();
    glbinding::initialize(sf::Context::getFunction);

#ifndef NDEBUG
    gl::glEnable(gl::GL_DEBUG_OUTPUT);
    gl::glDebugMessageCallback(
        [](gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity, gl::GLsizei, const gl::GLchar* message, const void*) {
            if (type != gl::GLenum::GL_DEBUG_TYPE_ERROR)
                return;
            throw std::runtime_error(std::format(R"(GL ERROR::
    source:     0x{:x}
    type:       0x{:x}
    id:         0x{:x}
    severity:   0x{:x}
    message:    {:s})",
                (uint32_t)source, (uint32_t)type, (uint32_t)id, (uint32_t)severity, message));
        },
        nullptr);

#endif
}

