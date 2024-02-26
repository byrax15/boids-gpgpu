module;
#include <glbinding/gl46core/gl.h>
export module vertex_array;

import contexts;

export struct vertex_array {

    vertex_array(opengl const&) { gl::glCreateVertexArrays(1, &vao); }

    ~vertex_array() { gl::glDeleteVertexArrays(1, &vao); }

    vertex_array() = delete;
    vertex_array(vertex_array const&) = delete;
    vertex_array& operator=(vertex_array const&) = delete;

    operator gl::GLuint() const { return vao; }

private:
    gl::GLuint vao = 0;
};