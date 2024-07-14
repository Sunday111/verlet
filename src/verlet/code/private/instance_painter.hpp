#pragma once

#include <array>
#include <memory>

#include "EverydayTools/Math/IntRange.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/template/type_to_gl_type.hpp"

namespace verlet
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class InstancedPainter
{
public:
    static constexpr GLuint kVertexAttribLoc = 0;       // not instanced
    static constexpr GLuint kColorAttribLoc = 1;        // instanced
    static constexpr GLuint kTranslationAttribLoc = 2;  // instanced
    static constexpr GLuint kScaleAttribLoc = 3;        // instanced

    struct Batch
    {
        Batch() = default;
        Batch(const Batch&) = delete;
        Batch(Batch&&) noexcept = default;

        static constexpr size_t kBatchSize = 128;

        template <typename ValueType>
        static void UpdateVBO(
            std::optional<GLuint>& vbo,
            const GLuint location,
            const std::array<ValueType, kBatchSize>& values,
            const edt::IntRange<size_t> elements_to_update,
            bool normalize_values)
        {
            const bool must_initialize = !vbo.has_value();
            if (must_initialize) vbo = klgl::OpenGl::GenBuffer();

            using GlTypeTraits = klgl::TypeToGlType<ValueType>;

            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *vbo);
            if (must_initialize)
            {
                // Have to copy the whole array first time to initialize the buffer and specify usage
                klgl::OpenGl::BufferData(GL_ARRAY_BUFFER, std::span{values}, GL_DYNAMIC_DRAW);
            }
            else
            {
                glBufferSubData(
                    GL_ARRAY_BUFFER,
                    static_cast<GLintptr>(elements_to_update.begin * sizeof(ValueType)),
                    static_cast<GLintptr>(elements_to_update.Extent() * sizeof(ValueType)),
                    &values[elements_to_update.begin]);
            }
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, 0);

            klgl::OpenGl::EnableVertexAttribArray(location);
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *vbo);
            klgl::OpenGl::VertexAttribPointer(
                location,
                GlTypeTraits::Size,
                GlTypeTraits::Type,
                normalize_values,
                sizeof(ValueType),
                nullptr);
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribDivisor(
                location,
                1);  // IMPORTANT - use 1 element from offsets array for one rendered instance
        }

        void UpdateColorsVBO(const edt::IntRange<size_t> elements_to_update = {0uz, kBatchSize})
        {
            UpdateVBO(opt_color_vbo, kColorAttribLoc, color, elements_to_update, true);
        }

        void UpdateTranslationsVBO(const edt::IntRange<size_t> elements_to_update = {0uz, kBatchSize})
        {
            UpdateVBO(opt_translation_vbo, kTranslationAttribLoc, translation, elements_to_update, false);
        }

        void UpdateScaleVBO(const edt::IntRange<size_t> elements_to_update = {0uz, kBatchSize})
        {
            UpdateVBO(opt_scale_vbo, kScaleAttribLoc, scale, elements_to_update, false);
        }

        std::optional<GLuint> opt_color_vbo{};
        std::array<Vec3<uint8_t>, kBatchSize> color{};

        std::optional<GLuint> opt_translation_vbo{};
        std::array<Vec2f, kBatchSize> translation{};

        std::optional<GLuint> opt_scale_vbo{};
        std::array<Vec2f, kBatchSize> scale{};
    };

    void SetCircle(const size_t index, const Vec2f& translation, const Vec3<uint8_t>& color, const Vec2f scale)
    {
        SetColor(index, color);
        SetScale(index, scale);
        SetTranslation(index, translation);
    }

    void SetColor(const size_t index, const Vec3<uint8_t>& color)
    {
        auto [batch, index_in_batch] = DecomposeIndex(index);
        assert(
            index >= num_initialized_ ||
            color == batch.color[index_in_batch]);  // In this app color for circle is immutable
        batch.color[index_in_batch] = color;
    }

    void SetScale(const size_t index, const Vec2f& scale)
    {
        auto [batch, index_in_batch] = DecomposeIndex(index);
        assert(
            index >= num_initialized_ ||
            scale == batch.scale[index_in_batch]);  // In this app scale for circle is immutable
        batch.scale[index_in_batch] = scale;
    }

    void SetTranslation(const size_t index, const Vec2f& translation)
    {
        auto [batch, index_in_batch] = DecomposeIndex(index);
        batch.translation[index_in_batch] = translation;
    }

    void Initialize();
    void Render();

    // Want to keep it inline
    std::tuple<Batch&, size_t> DecomposeIndex(const size_t index)
    {
        const size_t batch_index = index / Batch::kBatchSize;
        const size_t index_in_batch = index % Batch::kBatchSize;
        num_circles_ = std::max(num_circles_, index + 1);
        batches_.resize(std::max(batches_.size(), batch_index + 1));
        return {batches_[batch_index], index_in_batch};
    }

    std::unique_ptr<klgl::MeshOpenGL> mesh_;
    std::vector<Batch> batches_;
    size_t num_initialized_ = 0;
    size_t num_circles_ = 0;
};

}  // namespace verlet
