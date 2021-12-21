#version 330 core
out vec4 FragColor;
in float lenColorScale;
void main()
{
   FragColor = vec4(1.0f, 1.0f, 1.0f, lenColorScale);
}