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
    vec3 normal = vec3(p.x, p.y, sqrt(1 - radius2));
    // some arbitrary light position, maybe use a uniform?
    vec3 lightdir = normalize(vec3(0.25, -0.25, 1.0));
    if (2 * capture - 1.0 > -p.y) {
      color = cap_color;
    }
    float highlight = max(0, dot(normal, lightdir));

    gl_FragColor = color + highlight * vec4(0.5);
}
