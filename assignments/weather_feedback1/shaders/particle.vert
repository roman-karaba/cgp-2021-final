#version 330 core
layout (location = 0) in vec3 pos;


out vec4 vtxColor;

uniform mat4 viewProj;
uniform mat4 prevViewProj;
uniform vec3 offsets;
uniform vec3 cameraPos;
uniform vec3 forwardOffset;
uniform bool isPoint;
uniform vec3 velocity;
uniform float heightSize;


const float boxSize = 30;

void main()
{
    if(isPoint){
    vec3 position = mod(pos+offsets, boxSize);

    
    vec3 finalPos = position + forwardOffset + cameraPos - boxSize/2;
   

    gl_Position = viewProj * vec4(finalPos, 1.0);
    gl_PointSize = max( 10 / gl_Position.w, 1);
    vtxColor = vec4(1,1,1,1);
    }
    else{
        vec3 position = mod(pos+offsets, boxSize);
        vec3 finalPos = position + forwardOffset + cameraPos - boxSize/2;
        
        vec3 prevPos = finalPos + velocity * heightSize;
        
        vec4 bottom =  viewProj * vec4(finalPos, 1.0);
        
        vec4 top =  viewProj * vec4(prevPos, 1.0);
        vec4 topPrev =  prevViewProj * vec4(prevPos, 1.0);
        
        vec2 dir = (top.xy/top.w) - (bottom.xy/bottom.w);
        vec2 dirPrev = (topPrev.xy/topPrev.w) - (bottom.xy/bottom.w);
        
        float len = length(dir);
        float lenPrev = length(dirPrev);
        
        gl_Position = mix(topPrev, bottom, gl_VertexID % 2);
        vtxColor = vec4(1,1,1,(len/lenPrev));
    }
}
