//
// Created by byrax on 2024-02-24.
//

module;
#include <algorithm>
#include <format>
#include <fstream>
#include <glbinding/gl46core/gl.h>
#include <numeric>
#include <sstream>
#include <string_view>
export module shader;

import contexts;

export struct shader_builder {
    std::string_view filename;
    gl::GLenum type;
    std::vector<gl::GLuint> specializations {};
};

struct shader {
    shader(shader_builder const& b)
    {
        const auto& [filename, shader_t, specializations] = b;
        std::stringstream filecontent;
        filecontent << std::ifstream { filename.data(), std::ios::binary | std::ios::in }.rdbuf();
        const auto spirv = filecontent.str();

        shader_id = gl::glCreateShader(shader_t);
        gl::glShaderBinary(1, &shader_id, gl::GL_SHADER_BINARY_FORMAT_SPIR_V,
            spirv.data(), static_cast<gl::GLsizei>(spirv.size()));

        std::vector<gl::GLuint> spec_indices(specializations.size());
        std::iota(spec_indices.begin(), spec_indices.end(), 0);
        gl::glSpecializeShader(shader_id, "main", specializations.size(), spec_indices.data(), specializations.data());
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

export struct shader_program {
    explicit shader_program(opengl const&, std::initializer_list<shader_builder> builders)
    {
        std::vector<shader> shaders;
        std::transform(builders.begin(), builders.end(), std::back_inserter(shaders), [](shader_builder const& b) {
            return shader(b);
        });

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
