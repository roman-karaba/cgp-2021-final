#version 330 core
layout (location = 0) in vec3 particlePos; //particle position
layout (location = 1) in vec4 particleColor; //particle color

out vec4 particleVtxColor; //exposed color to fragment shader
out float lenColorScale; //exposed alpha to fragment shader


uniform mat4 particleModel;
uniform mat4 particleModelPrev;

uniform vec3 cameraPos; //camera position.
uniform vec3 camDirection; //camera direction.
uniform vec3 offsets; // position offsets.
uniform float boxSize; //box size of particles.


uniform float particlePointSize; // fixed size based on particle type
uniform float particleCameraDistance; //particle distance from camera

uniform float lineScaleFactor; // scale factor for lines.
uniform int isParticle; // flag to check lines or particles rendering mode.

uniform vec3 particleGravity;
uniform vec3 particleVelocity;
uniform vec3 particleRandom;

out vec3 debugColor;

void main()
{
   //Compute Particle Position
   
   //Particles
   vec3 tempCurrentPos = mod(particlePos + offsets, vec3(boxSize, boxSize, boxSize));
   tempCurrentPos += cameraPos + camDirection - vec3(boxSize, boxSize, boxSize)/2; //final computed position

	if(isParticle == 1)
		gl_Position = particleModel * vec4(tempCurrentPos,1.0); //POINTS ONLY

   else{
	   //Lines
	   vec4 particleDirection = normalize(vec4(particleGravity,1.0) + vec4(particleVelocity,1.0) + vec4(particleRandom,1.0));

	   vec4 worldPosPrev = vec4(tempCurrentPos,1.0) - particleDirection * lineScaleFactor;
	   worldPosPrev.w = 1.0f;

	   vec4 bottom = particleModel * vec4(tempCurrentPos,1.0);
	   vec4 top = particleModel * worldPosPrev;
	   vec4 topPrev = particleModelPrev * worldPosPrev;

	   //Calculating variables for alpha blending
	   vec2 dir = (top.xy/top.w) - (bottom.xy/bottom.w);
	   vec2 dirPrev = (topPrev.xy/topPrev.w) - (bottom.xy/bottom.w);

	   float len = length(dir);
	   float lenPrev = length(dirPrev);

	   //Calculate the alpha channel before sending to fragment shader.
	   lenColorScale = clamp(len/lenPrev, 0.0, 1.0);

	   //Calculate Line Position
	   gl_Position = mix(topPrev, bottom, mod(gl_VertexID, 2)); //LINES ONLY
	   particleVtxColor = vec4(particleDirection.xyz, 1.0f);
   }

	//Color
	particleVtxColor = particleColor; // pass color to fragment shader.

   vec3 particlePos = gl_Position.xyz;
   gl_PointSize = 1.0f + particlePointSize + (0.01f/(0.01f + distance(particlePos, cameraPos))); //computed size of point.
}