#pragma once

#include <array>
#include <memory>

#include "EverydayTools/Math/IntRange.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "mesh_vertex.hpp"

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
            const IntRange<size_t> elements_to_update)
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
                false,
                sizeof(ValueType),
                nullptr);
            klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribDivisor(
                location,
                1);  // IMPORTANT - use 1 element from offsets array for one rendered instance
        }

        void UpdateColorsVBO(const IntRange<size_t> elements_to_update = IntRange{0uz, kBatchSize});
        std::optional<GLuint> opt_color_vbo{};
        std::array<Vec3f, kBatchSize> color{};

        void UpdateTranslationsVBO(const IntRange<size_t> elements_to_update = IntRange{0uz, kBatchSize});
        std::optional<GLuint> opt_translation_vbo{};
        std::array<Vec2f, kBatchSize> translation{};

        void UpdateScaleVBO(const IntRange<size_t> elements_to_update = IntRange{0uz, kBatchSize});
        std::optional<GLuint> opt_scale_vbo{};
        std::array<Vec2f, kBatchSize> scale{};
    };

    void SetCircle(const size_t index, const Vec2f& translation, const Vec3f& color, const Vec2f scale)
    {
        SetColor(index, color);
        SetScale(index, scale);
        SetTranslation(index, translation);
    }

    void SetColor(const size_t index, const Vec3f& color)
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

    void Initialize()
    {
        const std::array<MeshVertex, 4> vertices{
            {{.position = {1.0f, 1.0f}},
             {.position = {1.0f, -1.0f}},
             {.position = {-1.0f, -1.0f}},
             {.position = {-1.0f, 1.0f}}}};
        const std::array<uint32_t, 6> indices{0, 1, 3, 1, 2, 3};

        mesh_ = klgl::MeshOpenGL::MakeFromData<MeshVertex>(std::span{vertices}, std::span{indices});
        mesh_->Bind();
        RegisterAttribute<&MeshVertex::position>(0, false);
    }

    void Render()
    {
        mesh_->Bind();
        for (const size_t batch_index : std::views::iota(0uz, batches_.size()))
        {
            auto& batch = batches_[batch_index];
            // number of circles initialized for the current batch
            const size_t num_locally_initialized = num_initialized_ % batch.kBatchSize;
            const size_t num_locally_used = std::min(num_circles_ - batch_index * batch.kBatchSize, batch.kBatchSize);

            // if we have new elements since last render - send color and scale for new elements
            if (num_locally_used > num_locally_initialized)
            {
                const IntRange update_range{num_locally_initialized, num_locally_used};
                batch.UpdateColorsVBO(update_range);
                batch.UpdateScaleVBO(update_range);
            }
            else
            {
                klgl::OpenGl::EnableVertexAttribArray(kColorAttribLoc);
                klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_color_vbo);
                klgl::OpenGl::EnableVertexAttribArray(kScaleAttribLoc);
                klgl::OpenGl::BindBuffer(GL_ARRAY_BUFFER, *batch.opt_scale_vbo);
            }

            // Update all offsets
            batch.UpdateTranslationsVBO({0, num_locally_used});
            mesh_->DrawInstanced(num_locally_used);
        }
    }

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
