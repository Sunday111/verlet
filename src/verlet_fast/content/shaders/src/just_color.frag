in vec3 Color;
out vec4 FragColor;

void main()
{
    FragColor.xyz = Color;
    FragColor.a = 1;
}
