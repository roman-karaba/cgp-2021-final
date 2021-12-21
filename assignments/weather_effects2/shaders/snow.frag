#version 330 core
out vec4 fragColor;

void main()
{
    float dist = distance(gl_PointCoord, vec2(0.5, 0.5));
    float alpha = (1 - 2*dist);
    if(dist > 0.5)
        alpha = 0;
    fragColor = vec4(1.0, 1.0, 1.0, alpha);
}