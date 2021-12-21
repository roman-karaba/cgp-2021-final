#version 330 core

out vec4 fragColor;
in float alpha;

void main()
{
    fragColor = vec4(vec3(1, 1, 1), 0.f + alpha);
}