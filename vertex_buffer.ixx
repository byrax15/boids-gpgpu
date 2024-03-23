module;
#include <glbinding/gl46core/gl.h>
#include <span>
#include <type_traits>
export module vertex_buffer;

import contexts;

template <typename T>
struct mapped_buffer : std::span<T> {

    mapped_buffer(gl::GLuint vbo, gl::GLuint size)
        : std::span<T> { (T*)gl::glMapNamedBuffer(vbo, gl::GL_READ_WRITE), size }
        , vbo(vbo)
    {
    }
    ~mapped_buffer() { gl::glUnmapNamedBuffer(vbo); }

    mapped_buffer(mapped_buffer const&) = delete;
    mapped_buffer& operator=(mapped_buffer const&) = delete;
    mapped_buffer(mapped_buffer&& old) noexcept
    {
        vbo = old.vbo;
        old.vbo = 0;
    }
    mapped_buffer& operator=(mapped_buffer&& old) noexcept
    {
        vbo = old.vbo;
        old.vbo = 0;
    }

private:
    gl::GLuint vbo;
};

export template <typename TContained>
struct vertex_buffer {
    using buffer_t = TContained;

    static constexpr auto buffer_t_sizeof() { return sizeof buffer_t; }

    vertex_buffer(opengl const&, gl::GLuint size)
        : m_size(size)
    {
        gl::glCreateBuffers(1, &m_vbo);
    }

    template <typename UContained, size_t Extent>
    vertex_buffer(opengl const& context, std::span<UContained, Extent> data)
        : vertex_buffer(context, data.size())
    {
        static_assert(std::is_same_v<TContained, std::remove_cvref_t<UContained>>);
        gl::glNamedBufferData(m_vbo, data.size_bytes(), data.data(), gl::GL_STATIC_DRAW);
    }

    ~vertex_buffer() { gl::glDeleteBuffers(1, &m_vbo); }

    vertex_buffer() = default;
    vertex_buffer(vertex_buffer const&) = delete;
    vertex_buffer& operator=(vertex_buffer const&) = delete;
    vertex_buffer(vertex_buffer&& moved_from) noexcept
    {
        m_vbo = moved_from.m_vbo;
        moved_from.m_vbo = 0;
    }
    vertex_buffer& operator=(vertex_buffer&& moved_from)
    {
        m_vbo = moved_from.m_vbo;
        moved_from.m_vbo = 0;
    }

    auto map_buffer() const { return mapped_buffer<buffer_t> { m_vbo, m_size }; }

    operator gl::GLuint() const { return m_vbo; }
    auto size() const { return m_size; }
    auto size_bytes() const { return m_size * sizeof(buffer_t); }

private:
    gl::GLuint m_vbo = 0;
    gl::GLuint m_size = 0;
};

template <typename TContained>
vertex_buffer(opengl const&, TContained&&) -> vertex_buffer<std::remove_cvref_t<TContained>>;

export template <typename TContained, size_t Extent>
vertex_buffer(opengl const&, std::span<TContained, Extent>) -> vertex_buffer<std::remove_cvref_t<TContained>>;
