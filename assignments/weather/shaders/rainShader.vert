#version 330 core
layout (location = 0) in vec3 pos;

out float lenColorScale;

uniform vec3 offset;
uniform vec3 velocity;
uniform float heightScale;

uniform vec3 camPosition;
uniform vec3 fwdOffset;
uniform float boxSize;

uniform mat4 viewproj;
uniform mat4 viewprojPrev;

void main()
{
   vec3 worldPos = mod(pos + offset, boxSize);
   worldPos += camPosition + fwdOffset - 0.5f * boxSize;
   vec3 worldPosPrev = worldPos + velocity * heightScale;

   vec4 bottom = viewproj * vec4(worldPos, 1.0);
   vec4 top = viewproj * vec4(worldPosPrev, 1.0);
   vec4 topPrev = viewprojPrev * vec4(worldPosPrev, 1.0);

   vec2 dir = (top.xy /top.w) - (bottom.xy/bottom.w);
   vec2 dirPrev = (topPrev.xy /topPrev.w) - (bottom.xy/bottom.w);

   gl_Position = mix(topPrev, bottom, gl_VertexID % 2);
   lenColorScale = length(dir)/length(dirPrev) / gl_Position.w;
}