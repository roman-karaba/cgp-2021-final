#version 330 core
out vec4 FragColor;
in vec3 vtxPos;
in vec3 vtxPosVS;
in vec3 vtxNormal;

uniform vec3 sunLightDirection;
uniform vec3 sunLightDiffuseColor;
uniform vec3 sunLightSpecular;
uniform vec3 sunLightAmbient;
uniform mat4 viewMatrix;
uniform float sunLightIntensity;

vec4 colorWater = vec4(0, 0, .6, 0);
vec4 colorGrass = vec4(0, .6, 0, 0);
vec4 colorStone= vec4(.4, .4, .4 ,0);

void main()
{
   vec4 color;
   if (vtxPos.y < 0) color = colorWater;
   else if (vtxPos.y > 10) color = colorStone;
   else color = colorGrass;

   float diffuse = max(dot(-sunLightDirection, vtxNormal), 0);
   vec3 diffuseContribution = sunLightDiffuseColor * diffuse;

   vec3 viewDirection = normalize(vtxPosVS);
   vec3 sunLightDirectionVS = (viewMatrix * vec4(sunLightDirection,0)).xyz;

   sunLightDirectionVS = normalize(sunLightDirectionVS);
   vec3 reflectDirection = reflect(sunLightDirectionVS, vtxNormal);

   float specular = pow( max( dot(viewDirection, reflectDirection), 0), 32 );
   vec3 specularContribution = 0.5 * specular * sunLightSpecular;

   vec3 finalColor = (sunLightAmbient * sunLightIntensity + specularContribution + diffuseContribution) * color.xyz ;
   FragColor = vec4(finalColor,1);
}