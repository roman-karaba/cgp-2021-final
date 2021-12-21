#version 330 core
layout (location = 0) in vec3 pos;

uniform vec3 offset;

uniform vec3 camPosition;
uniform vec3 fwdOffset;
uniform float boxSize;

uniform mat4 viewproj;

void main()
{
   float minSize = 0.1f;
   float maxSize = 10.0f;

   vec3 offsetPos = mod(pos + offset, boxSize);
   offsetPos += camPosition + fwdOffset - 0.5f * boxSize;

   gl_Position = viewproj * vec4(offsetPos, 1.0);

   gl_PointSize = max(maxSize / gl_Position.w, minSize);
}