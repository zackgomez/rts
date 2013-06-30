#version 120

uniform vec4 player_color;
uniform vec4 cap_color;
uniform float capture;
varying vec2 frag_texcoord;

const float PI = 3.1415926;

void main()
{
    vec4 color = player_color;
    vec2 p = 2 * (frag_texcoord - vec2(0.5));
    float radius2 = dot(p, p);
    if (radius2 > 1) discard;
    if (2 * capture - 1.0 > -p.y) {
      color = cap_color;
    }

    gl_FragColor = color;
}
