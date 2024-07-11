#include "klgl/opengl/gl_api.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "klgl/opengl/debug/annotations.hpp"

namespace klgl
{

template <typename T>
void GenObjects(T api_fn, const std::span<GLuint>& objects)
{
    api_fn(static_cast<GLsizei>(objects.size()), objects.data());
}

template <typename T>
GLuint GenObject(T api_fn)
{
    GLuint object{};
    GenObjects(api_fn, std::span(&object, 1));
    return object;
}

GLuint OpenGl::GenVertexArray() noexcept
{
    return GenObject(glGenVertexArrays);
}

void OpenGl::GenVertexArrays(const std::span<GLuint>& arrays) noexcept
{
    GenObjects(glGenVertexArrays, arrays);
}

GLuint OpenGl::GenBuffer() noexcept
{
    return GenObject(glGenBuffers);
}

void OpenGl::GenBuffers(const std::span<GLuint>& buffers) noexcept
{
    return GenObjects(glGenBuffers, buffers);
}

GLuint OpenGl::GenTexture() noexcept
{
    return GenObject(glGenTextures);
}

void OpenGl::GenTextures(const std::span<GLuint>& textures) noexcept
{
    GenObjects(glGenTextures, textures);
}

void OpenGl::BindVertexArray(GLuint array) noexcept
{
    glBindVertexArray(array);
}

void OpenGl::BindBuffer(GLenum target, GLuint buffer) noexcept
{
    glBindBuffer(target, buffer);
}

void OpenGl::BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) noexcept
{
    glBufferData(target, size, data, usage);
}

constexpr GLboolean OpenGl::CastBool(bool value) noexcept
{
    return static_cast<GLboolean>(value);
}

void OpenGl::VertexAttribPointer(
    GLuint index,
    size_t size,
    GLenum type,
    bool normalized,
    size_t stride,
    const void* pointer) noexcept
{
    glVertexAttribPointer(
        index,
        static_cast<GLint>(size),
        type,
        CastBool(normalized),
        static_cast<GLsizei>(stride),
        pointer);
}

void OpenGl::EnableVertexAttribArray(GLuint index) noexcept
{
    glEnableVertexAttribArray(index);
}

void OpenGl::Viewport(GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
    glViewport(x, y, width, height);
}

void OpenGl::SetClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) noexcept
{
    glClearColor(red, green, blue, alpha);
}
void OpenGl::SetClearColor(const Vec4f& color) noexcept
{
    SetClearColor(color.x(), color.y(), color.z(), color.w());
}

void OpenGl::Clear(GLbitfield mask) noexcept
{
    glClear(mask);
}

void OpenGl::UseProgram(GLuint program) noexcept
{
    glUseProgram(program);
}

void OpenGl::DrawElements(GLenum mode, size_t num, GLenum indices_type, const void* indices) noexcept
{
    ScopeAnnotation annotation("OpenGl::Draw");
    glDrawElements(mode, static_cast<GLsizei>(num), indices_type, indices);
}

std::optional<uint32_t> OpenGl::FindUniformLocation(GLuint shader_program, const char* name) noexcept
{
    int result = glGetUniformLocation(shader_program, name);
    [[likely]] if (result >= 0)
    {
        return static_cast<uint32_t>(result);
    }
    return std::optional<uint32_t>();
}

uint32_t OpenGl::GetUniformLocation(GLuint shader_program, const char* name)
{
    auto location = FindUniformLocation(shader_program, name);
    [[likely]] if (location.has_value())
    {
        return *location;
    }

    throw std::invalid_argument(fmt::format("Uniform with name {} was not found", name));
}

void OpenGl::SetUniform(uint32_t location, const float& f) noexcept
{
    glUniform1f(static_cast<GLint>(location), f);
}

void OpenGl::SetUniform(uint32_t location, const Mat4f& m, bool transpose) noexcept
{
    glUniformMatrix4fv(static_cast<GLint>(location), 1, CastBool(transpose), m.data());
}

void OpenGl::SetUniform(uint32_t location, const Mat3f& m, bool transpose) noexcept
{
    glUniformMatrix3fv(static_cast<GLint>(location), 1, CastBool(transpose), m.data());
}
void OpenGl::EnableDepthTest() noexcept
{
    glEnable(GL_DEPTH_TEST);
}

void OpenGl::SetUniform(uint32_t location, const Vec4f& v) noexcept
{
    glUniform4f(static_cast<GLint>(location), v.x(), v.y(), v.z(), v.w());
}

void OpenGl::SetUniform(uint32_t location, const Vec3f& v) noexcept
{
    glUniform3f(static_cast<GLint>(location), v.x(), v.y(), v.z());
}

void OpenGl::SetUniform(uint32_t location, const Vec2f& v) noexcept
{
    glUniform2f(static_cast<GLint>(location), v.x(), v.y());
}

void OpenGl::SetTextureParameter(GLenum target, GLenum pname, const GLfloat* value) noexcept
{
    glTexParameterfv(target, pname, value);
}

void OpenGl::SetTextureParameter(GLenum target, GLenum name, GLint param) noexcept
{
    glTexParameteri(target, name, param);
}

void OpenGl::SetTexture2dBorderColor(const Vec4f& v) noexcept
{
    SetTextureParameter2d(GL_TEXTURE_BORDER_COLOR, v.data());
}

void OpenGl::SetTexture2dWrap(GlTextureWrap wrap, GlTextureWrapMode mode) noexcept
{
    SetTextureParameter2d(ConvertEnum(wrap), ConvertEnum(mode));
}

void OpenGl::SetTexture2dMinFilter(GlTextureFilter filter) noexcept
{
    SetTextureParameter2d(GL_TEXTURE_MIN_FILTER, ConvertEnum(filter));
}

void OpenGl::SetTexture2dMagFilter(GlTextureFilter filter) noexcept
{
    SetTextureParameter2d(GL_TEXTURE_MAG_FILTER, ConvertEnum(filter));
}

void OpenGl::BindTexture(GLenum target, GLuint texture) noexcept
{
    glBindTexture(target, texture);
}

void OpenGl::BindTexture2d(GLuint texture)
{
    BindTexture(GL_TEXTURE_2D, texture);
}

void OpenGl::TexImage2d(
    GLenum target,
    size_t level_of_detail,
    GLint internal_format,
    size_t width,
    size_t height,
    GLenum data_format,
    GLenum pixel_data_type,
    const void* pixels) noexcept
{
    glTexImage2D(
        target,
        static_cast<GLint>(level_of_detail),
        internal_format,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        data_format,
        pixel_data_type,
        pixels);
}

void OpenGl::GenerateMipmap(GLenum target) noexcept
{
    glGenerateMipmap(target);
}

void OpenGl::GenerateMipmap2d() noexcept
{
    GenerateMipmap(GL_TEXTURE_2D);
}

void OpenGl::PolygonMode(GlPolygonMode mode) noexcept
{
    const GLenum converted = ConvertEnum(mode);
    glPolygonMode(GL_FRONT_AND_BACK, converted);
}

void OpenGl::PointSize(float size) noexcept
{
    glPointSize(size);
}
void OpenGl::LineWidth(float width) noexcept
{
    glLineWidth(width);
}

}  // namespace klgl
