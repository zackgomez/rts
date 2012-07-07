#version 120

varying vec4 fragPos;

uniform vec4 color;

float PI = 3.14159265358979323846264;

void main()
{
    float u = min(abs(sin(fragPos.x * PI)), abs(sin(fragPos.y * PI)));
    u = smoothstep(0, 0.1, u);
    gl_FragColor = u * color; //smoothstep(0.f, 0.1f, ) * color;
}

