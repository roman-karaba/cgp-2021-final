#version 330 core

in  vec4 particleVtxColor;
out vec4 FragColor;

in float lenColorScale;

void main()
{
   FragColor = vec4(particleVtxColor.rgb, 0.0f + lenColorScale);
}