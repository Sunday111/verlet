#include "instance_painter.hpp"

namespace verlet
{

void InstancedPainter::Batch::UpdateColorsVBO(const IntRange<size_t> elements_to_update)
{
    UpdateVBO(opt_color_vbo, kColorAttribLoc, color, elements_to_update);
}

void InstancedPainter::Batch::UpdateTranslationsVBO(const IntRange<size_t> elements_to_update)
{
    UpdateVBO(opt_translation_vbo, kTranslationAttribLoc, translation, elements_to_update);
}

void InstancedPainter::Batch::UpdateScaleVBO(const IntRange<size_t> elements_to_update)
{
    UpdateVBO(opt_scale_vbo, kScaleAttribLoc, scale, elements_to_update);
}
}  // namespace verlet
