layout(location = 0) in vec2 vertex_attribute;
layout(location = 1) in vec2 tex_coord_attribute;
layout(location = 2) in vec4 color_attribute;
layout(location = 3) in vec2 translation_attribute;
layout(location = 4) in vec2 scale_attribute;

out vec4 Color;
out vec2 TexCoord;

uniform mat3 u_world_to_view;

void main()
{
    vec2 screen_pos = (u_world_to_view * vec3(translation_attribute, 1)).xy;
    vec2 screen_size = (u_world_to_view * vec3(scale_attribute, 0)).xy;
    gl_Position = vec4(vertex_attribute * screen_size + screen_pos, 0.0, 1.0);
    Color = color_attribute;
    TexCoord = tex_coord_attribute;
}
