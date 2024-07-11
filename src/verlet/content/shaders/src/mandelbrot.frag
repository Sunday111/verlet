#define MAX_ITERATIONS 1000
#define COLORS_COUNT 10

uniform vec2 uCameraPos;
uniform vec2 uViewportSize;
uniform float uScale;
uniform vec3 uColorTable[COLORS_COUNT];
out vec4 fragColor;

vec3 GetColorByIndex(int index)
{
    return uColorTable[index];
}

vec3 ColorForIteration(int iteration)
{
    if (iteration == MAX_ITERATIONS)
        return vec3(0, 0, 0);

    const int segments_count = COLORS_COUNT - 1;
    const int iterations_per_segment = MAX_ITERATIONS / segments_count;
    int k = iteration * segments_count;
    int first_color_index = k / MAX_ITERATIONS;
    vec3 color_a = GetColorByIndex(first_color_index);
    vec3 color_b = GetColorByIndex(first_color_index + 1);
    float p = float(k % iterations_per_segment) / iterations_per_segment;
    return color_a + p * (color_b - color_a);
}

int DoMandelbrotLoop(vec2 p0)
{
    vec2 p = vec2(0, 0);
    int iteration = 0;
    while (dot(p, p) <= 4.0 && iteration != MAX_ITERATIONS)
    {
        float x_temp = p.x * p.x - p.y * p.y + p0.x;
        p.y = 2 * p.x * p.y + p0.y;
        p.x = x_temp;
        ++iteration;
    }
    return iteration;
}

void main()
{
    vec2 min_coord = vec2(-2.0, -2.0);
    vec2 max_coord = vec2(2.0, 2.0);

    if (uViewportSize.x > uViewportSize.y)
    {
        // extend x space
        float width = max_coord.x - min_coord.x;
        float center_x = min_coord.x + 0.5 * width;
        float x_scale = (float(uViewportSize.x) / float(uViewportSize.y));
        max_coord.x += 0.5 * width * (x_scale - 1.0);
        min_coord.x -= 0.5 * width * (x_scale - 1.0);

    } else {

    }

    vec2 range = uScale * (max_coord - min_coord);
    vec2 frag_pos = gl_FragCoord.xy / uViewportSize;
    vec2 coord = uCameraPos + range * (frag_pos - 0.5);
    int iterations = DoMandelbrotLoop(coord);
    fragColor.a = 1;
    fragColor.rgb = ColorForIteration(iterations);
}
