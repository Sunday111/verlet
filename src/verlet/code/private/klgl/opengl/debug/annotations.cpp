#include "klgl/opengl/debug/annotations.hpp"

#include "klgl/opengl/gl_api.hpp"

#define ENABLE_RENDERING_ANNOTATIONS

namespace klgl
{

ScopeAnnotation::ScopeAnnotation([[maybe_unused]] std::string_view scope_name, [[maybe_unused]] size_t id) noexcept
{
#ifdef ENABLE_RENDERING_ANNOTATIONS
    glPushDebugGroupKHR(
        GL_DEBUG_SOURCE_APPLICATION,
        static_cast<GLuint>(id),
        static_cast<GLsizei>(scope_name.size()),
        scope_name.data());
#endif
}

ScopeAnnotation::~ScopeAnnotation() noexcept
{
#ifdef ENABLE_RENDERING_ANNOTATIONS
    glPopDebugGroupKHR();
#endif
}

}  // namespace klgl
