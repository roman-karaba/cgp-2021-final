#version 330 core
out vec4 FragColor;
in vec3 vtxPos;
in vec3 normal;

uniform vec3 sunLightDirection;
uniform vec3 sunLightColor;
uniform float sunLightIntensity;

vec4 colorWater = vec4(0, 0, 1, 0);
vec4 colorGrass = vec4(0, 1, 0, 0);
vec4 colorStone= vec4(.7, .7, .7 ,0);

void main()
{
   vec4 color;
   if (vtxPos.y < 0) color = colorWater;
   else if (vtxPos.y > 10) color = colorStone;
   else color = colorGrass;

   float diff = dot(-sunLightDirection, normal);
   vec4 sunLightContribution = vec4(diff * sunLightColor, 0) * sunLightIntensity;

   FragColor = color + sunLightContribution;
}