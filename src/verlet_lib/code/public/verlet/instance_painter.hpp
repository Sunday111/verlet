#pragma once

#include <array>
#include <memory>

#include "EverydayTools/Math/IntRange.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/opengl/object.hpp"
#include "klgl/template/type_to_gl_type.hpp"

namespace klgl
{
class MeshOpenGL;
}  // namespace klgl

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class InstancedPainter
{
public:
    static constexpr GLuint kVertexAttribLoc = 0;       // same for all instances
    static constexpr GLuint kTexCoordAttribLoc = 1;     // same for all instances
    static constexpr GLuint kColorAttribLoc = 2;        // instanced
    static constexpr GLuint kTranslationAttribLoc = 3;  // instanced
    static constexpr GLuint kScaleAttribLoc = 4;        // instanced

    struct Batch
    {
        Batch() = default;
        Batch(const Batch&) = delete;
        Batch(Batch&&) noexcept = default;

        static constexpr size_t kBatchSize = 1 << 15;

        template <typename ValueType>
        static void UpdateVBO(
            klgl::GlObject<klgl::GlBufferId>& vbo,
            const GLuint location,
            const std::array<ValueType, kBatchSize>& values,
            const edt::IntRange<size_t> elements_to_update,
            bool normalize_values)
        {
            const bool must_initialize = !vbo.IsValid();
            if (must_initialize) vbo = klgl::GlObject<klgl::GlBufferId>::CreateFrom(klgl::OpenGl::GenBuffer());

            using GlTypeTraits = klgl::TypeToGlType<ValueType>;

            klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, vbo);
            if (must_initialize)
            {
                // Have to copy the whole array first time to initialize the buffer and specify usage
                klgl::OpenGl::BufferData(klgl::GlBufferType::Array, std::span{values}, klgl::GlUsage::DynamicDraw);
            }
            else
            {
                glBufferSubData(
                    GL_ARRAY_BUFFER,
                    static_cast<GLintptr>(elements_to_update.begin * sizeof(ValueType)),
                    static_cast<GLintptr>(elements_to_update.Extent() * sizeof(ValueType)),
                    &values[elements_to_update.begin]);
            }
            klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, {});

            klgl::OpenGl::EnableVertexAttribArray(location);
            klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, vbo);
            klgl::OpenGl::VertexAttribPointer(
                location,
                GlTypeTraits::Size,
                GlTypeTraits::AttribComponentType,
                normalize_values,
                sizeof(ValueType),
                nullptr);
            klgl::OpenGl::BindBuffer(klgl::GlBufferType::Array, {});
            glVertexAttribDivisor(
                location,
                1);  // IMPORTANT - use 1 element from offsets array for one rendered instance
        }

        void UpdateBuffers()
        {
            UpdateVBO(opt_color_vbo, kColorAttribLoc, color, dirty_colors, true);
            UpdateVBO(opt_translation_vbo, kTranslationAttribLoc, translation, dirty_translations, false);
            UpdateVBO(opt_scale_vbo, kScaleAttribLoc, scale, dirty_scales, false);
            dirty_colors = {0, 0};
            dirty_translations = {0, 0};
            dirty_scales = {0, 0};
        }

        template <std::ranges::random_access_range Collection, typename ValueType>
            requires(std::same_as<std::ranges::range_value_t<Collection>, ValueType>)
        static void SetValueAtIndexHelper(
            Collection& values,
            const ValueType& value,
            const size_t index,
            edt::IntRange<size_t>& dirty_range)
        {
            if (values[index] != value)
            {
                values[index] = value;
                if (dirty_range.begin == dirty_range.end)
                {
                    dirty_range = {index, index + 1};
                }
                else
                {
                    if (index >= dirty_range.end)
                    {
                        dirty_range.end = index + 1;
                    }
                    else if (index < dirty_range.begin)
                    {
                        dirty_range.begin = index;
                    }
                }
            }
        }

        void SetValue(const size_t index, const Vec4<uint8_t>& color_, const Vec2f& translation_, const Vec2f& scale_)
        {
            SetValueAtIndexHelper(color, color_, index, dirty_colors);
            SetValueAtIndexHelper(translation, translation_, index, dirty_translations);
            SetValueAtIndexHelper(scale, scale_, index, dirty_scales);
        }

        klgl::GlObject<klgl::GlBufferId> opt_color_vbo{};
        std::array<Vec4<uint8_t>, kBatchSize> color{};
        edt::IntRange<size_t> dirty_colors{0, 0};

        klgl::GlObject<klgl::GlBufferId> opt_translation_vbo{};
        std::array<Vec2f, kBatchSize> translation{};
        edt::IntRange<size_t> dirty_translations{0, 0};

        klgl::GlObject<klgl::GlBufferId> opt_scale_vbo{};
        std::array<Vec2f, kBatchSize> scale{};
        edt::IntRange<size_t> dirty_scales{0, 0};
    };

public:
    InstancedPainter();
    InstancedPainter(InstancedPainter&&) = delete;
    InstancedPainter(const InstancedPainter&) = delete;
    ~InstancedPainter();

    void DrawObject(const Vec2f& translation, const Vec4<uint8_t>& color, const Vec2f scale)
    {
        const size_t index = num_objects_++;
        auto [batch, index_in_batch] = DecomposeIndex(index);
        batch.SetValue(index_in_batch, color, translation, scale);
    }

    void Initialize();
    void Render();

    // Want to keep it inline
    std::tuple<Batch&, size_t> DecomposeIndex(const size_t index)
    {
        const size_t batch_index = index / Batch::kBatchSize;
        const size_t index_in_batch = index % Batch::kBatchSize;
        batches_.resize(std::max(batches_.size(), batch_index + 1));
        return {batches_[batch_index], index_in_batch};
    }

    std::unique_ptr<klgl::MeshOpenGL> mesh_;
    std::vector<Batch> batches_;
    size_t num_initialized_ = 0;
    size_t num_objects_ = 0;
};

}  // namespace verlet
