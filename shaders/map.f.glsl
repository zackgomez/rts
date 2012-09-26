#version 120

varying vec2 fragPos;

uniform vec4 color;
uniform vec2 gridDim;

float PI = 3.14159265358979323846264;

void main()
{
  vec2 gridPos = fragPos * gridDim;
  float u = min(abs(sin(gridPos.x * PI)), abs(sin(gridPos.y * PI)));
  u = smoothstep(0, 0.1, u);

  gl_FragColor = u * color;
}
