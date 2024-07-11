#pragma once

#include "klgl/opengl/gl_api.hpp"
#include "klgl/template/class_member_traits.hpp"
#include "klgl/template/member_offset.hpp"
#include "klgl/template/type_to_gl_type.hpp"
#include "matrix.hpp"

struct MeshVertex
{
    Vec2f position{};
};

template <auto MemberVariablePtr>
void RegisterAttribute(const GLuint location, const bool normalized)
{
    using MemberTraits = klgl::ClassMemberTraits<decltype(MemberVariablePtr)>;
    using GlTypeTraits = klgl::TypeToGlType<typename MemberTraits::Member>;
    const size_t vertex_stride = sizeof(typename MemberTraits::Class);
    const size_t member_stride = klgl::MemberOffset<MemberVariablePtr>();
    klgl::OpenGl::EnableVertexAttribArray(location);
    klgl::OpenGl::VertexAttribPointer(
        location,
        GlTypeTraits::Size,
        GlTypeTraits::Type,
        normalized,
        vertex_stride,
        reinterpret_cast<void*>(member_stride));  // NOLINT
}
