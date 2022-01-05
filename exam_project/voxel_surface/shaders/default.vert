#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 instancingOffsets;
out vec3 vtxPos;
out vec3 normal;
//out vec3 vtxPosVS;

//uniform mat4 viewMatrix;
uniform mat4 viewProjectionMatrix;
uniform vec3 chunkOffset;

void main()
{
   vec3 worldPos = pos+instancingOffsets + chunkOffset;
//   vtxPosVS = (viewMatrix * vec4(worldPos, 1)).xyz;
   gl_Position = viewProjectionMatrix * vec4(worldPos , 1.0);
   vtxPos = worldPos;
   normal = aNormal;
}