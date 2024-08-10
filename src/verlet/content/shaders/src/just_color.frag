in vec3 Color;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor.xyz = Color;
    FragColor *= texture(uTexture, TexCoord);
    FragColor.w = 1;
}
