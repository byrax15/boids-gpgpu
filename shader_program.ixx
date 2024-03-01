//
// Created by byrax on 2024-02-24.
//

module;
#include <algorithm>
#include <format>
#include <fstream>
#include <glbinding/gl46core/gl.h>
#include <sstream>
#include <string_view>
export module shader;

import contexts;

struct shader {
    shader(std::string_view filename, gl::GLenum shader_t)
    {
        std::stringstream filecontent;
        filecontent << std::ifstream { filename.data(), std::ios::binary | std::ios::in }.rdbuf();
        const auto spirv = filecontent.str();

        shader_id = gl::glCreateShader(shader_t);
        gl::glShaderBinary(1, &shader_id, gl::GL_SHADER_BINARY_FORMAT_SPIR_V,
            spirv.data(), static_cast<gl::GLsizei>(spirv.size()));
        gl::glSpecializeShader(shader_id, "main", 0, nullptr, nullptr);
    }

    shader(shader const&) = delete;
    shader& operator=(shader const&) = delete;
    shader(shader&& old) noexcept
    {
        shader_id = old.shader_id;
        old.shader_id = 0;
    };
    shader& operator=(shader&& old) noexcept
    {
        shader_id = old.shader_id;
        old.shader_id = 0;
    };

    [[nodiscard]]
    operator gl::GLuint() const { return shader_id; }

    ~shader() { gl::glDeleteShader(shader_id); }

private:
    gl::GLuint shader_id;
};

export struct shader_builder {
    std::string_view filename;
    gl::GLenum type;
};

export struct shader_program {
    explicit shader_program(opengl const&, std::initializer_list<shader_builder> builders)
    {
        std::vector<shader> shaders;
        std::transform(builders.begin(), builders.end(), std::back_inserter(shaders), [](shader_builder const& b) { return shader { b.filename, b.type }; });

        program_id = gl::glCreateProgram();
        std::ranges::for_each(shaders, [&](shader const& s) {
            gl::glAttachShader(program_id, s);
        });
        gl::glLinkProgram(program_id);
        std::ranges::for_each(shaders, [&](shader const& s) {
            gl::glDetachShader(program_id, s);
        });
    }

    ~shader_program()
    {
        gl::glDeleteProgram(program_id);
    }

    [[nodiscard]]
    operator gl::GLuint() const { return program_id; }

private:
    gl::GLuint program_id;
};
