#include "klgl/opengl/debug/gl_debug_messenger.hpp"

#include "EverydayTools/Preprocessor/Stringify.hpp"
#include "fmt/core.h"

namespace klgl
{

void GlDebugMessenger::Start()
{
    // glEnable(GL_DEBUG_OUTPUT);
    //  glDebugMessageCallback(GlDebugMessenger::DebugProc, nullptr);
    //  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

#define CASE_RET_STR(val, name) \
    case val:                   \
        return TOSTRING(name);  \
        break

static std::string_view SourceToString(GLenum source)
{
    switch (source)
    {
        CASE_RET_STR(GL_DEBUG_SOURCE_API, API);
        CASE_RET_STR(GL_DEBUG_SOURCE_WINDOW_SYSTEM, WINDOW_SYSTEM);
        CASE_RET_STR(GL_DEBUG_SOURCE_SHADER_COMPILER, SHADER_COMPILER);
        CASE_RET_STR(GL_DEBUG_SOURCE_THIRD_PARTY, THIRD_PARTY);
        CASE_RET_STR(GL_DEBUG_SOURCE_APPLICATION, APPLICATION);
        CASE_RET_STR(GL_DEBUG_SOURCE_OTHER, OTHER);
    default:
        assert(false);
        return "unknown";
        break;
    }
}

static std::string_view TypeToString(GLenum type)
{
    switch (type)
    {
        CASE_RET_STR(GL_DEBUG_TYPE_ERROR, ERROR);
        CASE_RET_STR(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, DEPRECATED_BEHAVIOR);
        CASE_RET_STR(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, UNDEFINED_BEHAVIOR);
        CASE_RET_STR(GL_DEBUG_TYPE_PORTABILITY, PORTABILITY);
        CASE_RET_STR(GL_DEBUG_TYPE_PERFORMANCE, PERFORMANCE);
        CASE_RET_STR(GL_DEBUG_TYPE_PUSH_GROUP, PUSH_GROUP);
        CASE_RET_STR(GL_DEBUG_TYPE_POP_GROUP, POP_GROUP);
        CASE_RET_STR(GL_DEBUG_TYPE_OTHER, OTHER);
    default:
        assert(false);
        return "unknown";
        break;
    }
}

void GlDebugMessenger::DebugProc(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum /*severity*/,
    [[maybe_unused]] GLsizei length,
    const GLchar* message,
    [[maybe_unused]] const void* user_param)
{
    fmt::println("{} {} {}: {}", SourceToString(source), TypeToString(type), id, message);
}

}  // namespace klgl
