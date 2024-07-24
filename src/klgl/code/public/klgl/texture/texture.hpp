#pragma once

#include <memory>
#include <optional>
#include <span>

#include "klgl/opengl/gl_api.hpp"

namespace klgl
{

class Texture
{
public:
    static std::unique_ptr<Texture> CreateEmpty(size_t width, size_t height, GLint internal_format = GL_RGB);

    ~Texture();

    void Bind() const;
    void SetPixels(std::span<const Vec3<uint8_t>> pixel_data);
    Vec2<size_t> GetSize() const { return {width_, height_}; }
    size_t GetWidth() const { return width_; }
    size_t GetHeight() const { return height_; }
    std::optional<GLuint> GetTexture() const { return texture_; }

private:
    std::optional<GLuint> texture_;
    size_t width_ = 0;
    size_t height_ = 0;
    GLenum type_ = GL_TEXTURE_2D;
};

}  // namespace klgl
