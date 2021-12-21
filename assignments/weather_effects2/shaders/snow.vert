#version 330 core
layout (location = 0) in vec3 pos;

uniform float boxSize;

uniform mat4 viewProj;
uniform mat4 viewProjPrev;

uniform vec3 cameraPos;
uniform vec3 forwardOffset;
uniform vec3 offsets;

uniform float maxSize;
const float minSize = 0.0f;

void main()
{
    vec3 newPos = mod(pos + offsets, boxSize) + cameraPos + forwardOffset - boxSize/2;
    gl_Position = viewProj * vec4(newPos, 1.0);
    gl_PointSize = max(maxSize / gl_Position.w, minSize);
}