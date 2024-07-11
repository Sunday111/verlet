#include "klgl/texture/texture.hpp"

#include <cassert>

namespace klgl
{

std::unique_ptr<Texture> Texture::CreateEmpty(size_t width, size_t height, GLint internal_format)
{
    assert(width != 0 && height != 0);
    auto tex = std::make_unique<Texture>();
    tex->texture_ = OpenGl::GenTexture();
    tex->type_ = GL_TEXTURE_2D;
    tex->width_ = width;
    tex->height_ = height;
    tex->Bind();
    GLenum pixel_data_format = GL_RGB;
    GLenum pixel_data_type = GL_UNSIGNED_BYTE;

    assert(tex->type_ == GL_TEXTURE_2D);
    // auto err = glGetError();
    // assert(err == GL_NO_ERROR);
    OpenGl::TexImage2d(tex->type_, 0, internal_format, width, height, pixel_data_format, pixel_data_type, nullptr);
    // err = glGetError();
    // assert(err == GL_NO_ERROR);

    OpenGl::SetTexture2dWrap(GlTextureWrap::R, GlTextureWrapMode::Repeat);
    OpenGl::SetTexture2dWrap(GlTextureWrap::S, GlTextureWrapMode::Repeat);
    OpenGl::SetTexture2dWrap(GlTextureWrap::T, GlTextureWrapMode::Repeat);
    OpenGl::SetTexture2dMagFilter(GlTextureFilter::Nearest);
    OpenGl::SetTexture2dMinFilter(GlTextureFilter::Nearest);

    return tex;
}

void Texture::Bind() const
{
    OpenGl::BindTexture(type_, *texture_);
}

void Texture::SetPixels(std::span<const Vec3<uint8_t>> pixel_data)
{
    assert(type_ == GL_TEXTURE_2D);
    assert(width_ * height_ == pixel_data.size());
    Bind();
    GLint x_offset = 0, y_offset = 0;
    GLenum pixel_data_format = GL_RGB;
    GLenum pixel_data_type = GL_UNSIGNED_BYTE;
    glTexSubImage2D(
        type_,
        0,
        x_offset,
        y_offset,
        static_cast<GLsizei>(width_),
        static_cast<GLsizei>(height_),
        pixel_data_format,
        pixel_data_type,
        pixel_data.data());
    assert(glGetError() == GL_NO_ERROR);

    std::vector<Vec3<uint8_t>> got_pixels;
    got_pixels.resize(pixel_data.size());
    glGetTexImage(GL_TEXTURE_2D, 0, pixel_data_format, pixel_data_type, got_pixels.data());
    [[maybe_unused]] auto err = glGetError();
    assert(err == GL_NO_ERROR);

    std::vector<size_t> different_idnices;
    for (size_t i = 0; i != got_pixels.size(); ++i)
    {
        if (pixel_data[i] != got_pixels[i])
        {
            different_idnices.push_back(i);
        }
    }
    assert(different_idnices.empty());
}

Texture::~Texture()
{
    if (texture_)
    {
        glDeleteTextures(1, &*texture_);
    }
}

}  // namespace klgl
