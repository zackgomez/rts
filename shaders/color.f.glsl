#version 330

in vec4 fragPos;
out vec4 outputColor;

uniform vec4 color;

float PI = 3.14159265358979323846264;

void main()
{
    float u = min(abs(sin(fragPos.x * PI)), abs(sin(fragPos.y * PI)));
    u = smoothstep(0, 0.1, u);
    outputColor = u * color; //smoothstep(0.f, 0.1f, ) * color;

    //float u = smoothstep(0.f, 0.1f, fragPos.x - floor(fragPos.x));
    //u *= smoothstep(0.f, 0.1f, fragPos.y - floor(fragPos.y));
}
