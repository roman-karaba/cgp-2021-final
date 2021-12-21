#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
   vec3 Position;
   vec3 Color;
   float Constant;
   float Linear;
   float Quadratic;
};
const int NR_LIGHTS = 128;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;
uniform float specularOffset;
uniform float lightIntensity;

uniform bool lightsAreOn;
uniform bool sharpen;
uniform bool edgeDetection;


void main()
{
   // retrieve data from gbuffer
   vec3 FragPos = texture(gPosition, TexCoords).rgb;
   vec3 Normal = texture(gNormal, TexCoords).rgb;
   vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
   float Specular = texture(gAlbedoSpec, TexCoords).a + specularOffset;

   vec2 texelSize = 1.0 / textureSize(gPosition, 0);
   vec2 up = vec2(0.0, texelSize.y);
   vec2 right = vec2(texelSize.x, 0.0);

   // more about convolution kernels
   // https://en.wikipedia.org/wiki/Kernel_(image_processing)
   // TODO ex 12.5 implement a sharpen filter to the diffuse and specular information of the gBuffer
   if (sharpen){
      // sharpen filter
      // should apply on samples from gAlbedoSpec texture
      vec4 sharp = vec4(Diffuse, Specular);
      Diffuse = sharp.rgb;
      Specular = sharp.a + specularOffset;
   }

   // TODO ex 12.6 implement an edge detection filter to the diffuse and specular information of the gBuffer
   if (edgeDetection){
      // laplacian operator
      // should apply on samples from gNormal
      float flatness = 1.0;// value in the range [0, 1], bigger values indicate less curvature
      Diffuse *= flatness;
      Specular *= flatness;
   }

   vec3 lighting = vec3(0,0,0);

   if (lightsAreOn){
      // calculate lighting
      lighting = Diffuse * 0.1; // hard-coded ambient component

      // TODO ex 12.4 compute the diffuse and specular components of the blinn-phong reflection model and
      //  distance attenuation for all lights in the scene.
      // You will need to iterate all lights (NR_LIGHTS), use forward_shading.frag as a reference.
      // Notice that all lights are point lights and that all information is in world coordinates
      // Forward and deferred shading should look identical after you implement lighting.


   }

   FragColor = vec4(lighting * lightIntensity , 1.0);
}

