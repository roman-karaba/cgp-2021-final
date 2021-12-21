#version 330 core
layout (location = 0) in vec3 pos;

uniform float boxSize;

uniform mat4 viewProj;
uniform mat4 viewProjPrev;

uniform vec3 cameraPos;
uniform vec3 forwardOffset;
uniform vec3 offsets;

uniform vec3 velocity;
uniform float heightScale;

out float lenColorScale;

void main()
{
    // Position
    vec3 newPos = mod(pos + offsets, boxSize) + cameraPos + forwardOffset - boxSize/2;
    vec3 oldPos = newPos + velocity * heightScale;
    vec4 bot = viewProj * vec4(newPos, 1.0);
    vec4 topPrev = viewProjPrev * vec4(oldPos, 1.0);
    gl_Position = mix(topPrev, bot, mod(gl_VertexID, 2));

    // Opacity
    vec4 top = viewProj * vec4(oldPos, 1.0);
    vec2 dir = (top.xy /top.w) - (bot.xy/bot.w);
    vec2 dirPrev = (topPrev.xy / topPrev.w) - (bot.xy/bot.w);
    lenColorScale = clamp(length(dir) / length(dirPrev), 0.0, 0.5);
}