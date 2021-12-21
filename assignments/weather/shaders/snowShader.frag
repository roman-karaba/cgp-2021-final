#version 330 core
out vec4 FragColor;
void main()
{
   float xDist = gl_PointCoord.x - 0.5f;
   float yDist = gl_PointCoord.y - 0.5f;
   float dist = sqrt(xDist * xDist + yDist * yDist);

   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f - dist * 2.0f);
}