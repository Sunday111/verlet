layout(location = 0) in vec2 vertex_attribute;
layout(location = 1) in vec2 tex_coord_attribute;
layout(location = 2) in vec3 color_attribute;
layout(location = 3) in vec2 translation_attribute;
layout(location = 4) in vec2 scale_attribute;

out vec3 Color;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(vertex_attribute * scale_attribute + translation_attribute, 0.0, 1.0);
    Color = color_attribute;
    TexCoord = tex_coord_attribute;
}
