in vec4 Color;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = Color * texture(uTexture, TexCoord);
}
