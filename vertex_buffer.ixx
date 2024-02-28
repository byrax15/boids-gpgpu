module;
#include <glbinding/gl46core/gl.h>
#include <span>
#include <type_traits>
export module vertex_buffer;

import contexts;

export template <typename TContained>
struct vertex_buffer {
    using buffer_t = TContained;

    vertex_buffer(opengl const&)
    {
        gl::glCreateBuffers(1, &vbo);
    }

    vertex_buffer(opengl const& context, TContained&& data)
        : vertex_buffer(context)
    {
        gl::glNamedBufferStorage(vbo, sizeof(TContained), &data, gl::GL_DYNAMIC_STORAGE_BIT);
    }

    template <typename UContained, size_t Extent>
    vertex_buffer(opengl const& context, std::span<UContained, Extent> data)
        : vertex_buffer(context)
    {
        static_assert(std::is_same_v<TContained, std::remove_cvref_t<UContained>>);
        gl::glNamedBufferStorage(vbo, data.size_bytes(), data.data(), gl::GL_DYNAMIC_STORAGE_BIT);
    }

    ~vertex_buffer() { gl::glDeleteBuffers(1, &vbo); }

    void bind_attribute(gl::GLuint attrib, gl::GLuint bind_slot, gl::GLuint divisor) const
    {
        gl::glBindVertexBuffer(bind_slot, vbo, 0, sizeof(buffer_t));
        gl::glEnableVertexAttribArray(attrib);
        gl::glVertexAttribFormat(attrib, buffer_t::length(), gl::GL_FLOAT, gl::GL_FALSE, 0);
        gl::glVertexBindingDivisor(bind_slot, divisor);
        gl::glVertexAttribBinding(attrib, bind_slot);
    }

    vertex_buffer() = default;
    vertex_buffer(vertex_buffer const&) = delete;
    vertex_buffer& operator=(vertex_buffer const&) = delete;
    vertex_buffer(vertex_buffer&& moved_from) noexcept
    {
        vbo = moved_from.vbo;
        moved_from.vbo = 0;
    }
    vertex_buffer& operator=(vertex_buffer&& moved_from)
    {
        vbo = moved_from.vbo;
        moved_from.vbo = 0;
    }

    operator gl::GLuint() const { return vbo; }

private:
    gl::GLuint vbo = 0;
};

template <typename TContained>
vertex_buffer(opengl const&, TContained&&) -> vertex_buffer<std::remove_cvref_t<TContained>>;

export template <typename TContained, size_t Extent>
vertex_buffer(opengl const&, std::span<TContained, Extent>) -> vertex_buffer<std::remove_cvref_t<TContained>>;
