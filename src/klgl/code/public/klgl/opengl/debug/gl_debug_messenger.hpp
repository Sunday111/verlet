#pragma once

#include "klgl/opengl/gl_api.hpp"

namespace klgl
{

class GlDebugMessenger
{
public:
    static void Start();

    static void DebugProc(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* user_param);
};

}  // namespace klgl
