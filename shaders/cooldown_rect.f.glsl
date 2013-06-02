#version 120

uniform vec4 color;
uniform float cooldown_percent;
varying vec2 frag_texcoord;

const float PI = 3.1415926;

void main()
{
    vec2 p = frag_texcoord - vec2(0.5);
    float angle = -atan(p.x, p.y) / (2 * PI) + 0.5;

    if (angle < cooldown_percent) {
      discard;
    }

    gl_FragColor = color;
}
