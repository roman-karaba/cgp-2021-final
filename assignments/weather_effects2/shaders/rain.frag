#version 330 core
out vec4 fragColor;
in float lenColorScale;

void main()
{
    fragColor = vec4(1.0, 1.0, 1.0, lenColorScale);
}