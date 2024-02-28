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

    void handle_events() const;
    void render_loop(auto&& callable) const;

private:
    mutable sf::Window window;
};

opengl::opengl()
    : window(sf::Window { sf::VideoMode(1920, 1080), "Boids", sf::Style::Default, std::invoke([] {
                             sf::ContextSettings settings;
                             settings.majorVersion = 4;
                             settings.minorVersion = 6;
                             return settings;
                         }) })
{
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

void opengl::handle_events() const
{
    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Resized:
            const auto [w, h] = event.size;
            gl::glViewport(0, 0, w, h);
            break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Escape)
                window.close();
            break;
        default:
            break;
        }
    }
}

void opengl::render_loop(auto&& callable) const
{
    while (window.isOpen()) {
        handle_events();
        window.display();
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        callable();
    }
}
