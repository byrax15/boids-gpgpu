//
// Created by byrax on 2024-02-24.
//

module;
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

        gl::GLint compiled = 0;
        gl::glGetShaderiv(shader_id, gl::GL_COMPILE_STATUS, &compiled);
        if (!compiled)
            throw std::runtime_error(std::format("OPENGL::Shader not compiled {}", filename));
    }

    [[nodiscard]]
    operator gl::GLuint() const { return shader_id; }

    ~shader() { gl::glDeleteShader(shader_id); }

private:
    gl::GLuint shader_id;
};

export struct shader_program {
    explicit shader_program(opengl const&, std::string_view vs_filename, std::string_view fs_filename)
    {

        const auto vs = shader { vs_filename, gl::GLenum::GL_VERTEX_SHADER };
        const auto fs = shader { fs_filename, gl::GLenum::GL_FRAGMENT_SHADER };

        program_id = gl::glCreateProgram();
        gl::glAttachShader(program_id, vs);
        gl::glAttachShader(program_id, fs);
        gl::glLinkProgram(program_id);

        gl::GLint linked = 0;
        gl::glGetProgramiv(program_id, gl::GL_LINK_STATUS, &linked);

        gl::glDetachShader(program_id, vs);
        gl::glDetachShader(program_id, fs);
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
