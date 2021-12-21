#version 330 core
layout (location = 0) in vec3 particlePosition;

uniform mat4 mvpMatrix;
uniform mat4 previousMvpMatrix;

uniform vec3 cameraPosition;
uniform vec3 forwardOffset;
uniform vec3 gravityVelocity;
uniform vec3 windVelocity;

uniform vec3 in_Offsets;
uniform float boxSize;
uniform int renderAsParticle;
uniform float particleScale;

out float alpha;
out vec3 debugColor;

void main()
{
    float lineScalar = 0.05f;
    vec3 updatedPosition = mod(particlePosition + in_Offsets, boxSize);
    updatedPosition += cameraPosition + forwardOffset - boxSize/2;

    if(renderAsParticle == 1)
    {
        gl_Position = mvpMatrix * vec4(updatedPosition, 1.0);
    }
    else // renderAsLines
    {
        vec4 particleDirection = normalize(vec4(windVelocity+gravityVelocity, 1.0));
        vec4 previousPosition = vec4(updatedPosition,1.0) + particleDirection * lineScalar;
        previousPosition.w = 1.0f;

        vec4 bottomVertex = mvpMatrix * vec4(updatedPosition,1.0);
        vec4 topVertex = mvpMatrix * previousPosition;
        vec4 prevTopVertex = previousMvpMatrix * previousPosition;
        vec2 velocity = (topVertex.xy/topVertex.w) - (bottomVertex.xy/bottomVertex.w);
        vec2 prevVelocity = (prevTopVertex.xy/prevTopVertex.w) - (bottomVertex.xy/bottomVertex.w);
        alpha = clamp(length(velocity)/length(prevVelocity), 0.0, 1.0);

        gl_Position = mix(prevTopVertex, bottomVertex, mod(gl_VertexID, 2));
    }

    gl_PointSize = particleScale;
}