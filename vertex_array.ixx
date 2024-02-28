module;
#include <glbinding/gl46core/gl.h>
#include <span>
export module vertex_array;

import contexts;

export struct vertex_attrib_format {
    gl::GLuint attribute_id, binding_id, divisor, array_size, relative_offset;
    gl::GLenum type;
    gl::GLboolean normalize;
};

export struct vertex_array {
    vertex_array(opengl const&)
    {
        gl::glCreateVertexArrays(1, &vao);
    }

    ~vertex_array() { gl::glDeleteVertexArrays(1, &vao); }

    vertex_array const& format_attrib(std::span<const vertex_attrib_format> formats) const
    {
        for (const auto& f : formats) {
            gl::glEnableVertexArrayAttrib(vao, f.attribute_id);
            gl::glVertexArrayAttribFormat(vao, f.attribute_id, f.array_size, f.type, f.normalize, f.relative_offset);
            gl::glVertexArrayBindingDivisor(vao, f.binding_id, f.divisor);
            gl::glVertexArrayAttribBinding(vao, f.attribute_id, f.binding_id);
        }
        return *this;
    }

    vertex_array() = delete;
    vertex_array(vertex_array const&) = delete;
    vertex_array& operator=(vertex_array const&) = delete;

    operator gl::GLuint() const { return vao; }

private:
    gl::GLuint vao = 0;
};