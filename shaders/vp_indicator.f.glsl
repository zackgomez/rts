#version 120

uniform vec4 player_color;
uniform vec4 cap_color;
uniform float capture;
uniform bool danger;
uniform float t;
varying vec2 frag_texcoord;

const float PI = 3.1415926;

void main()
{
    vec4 color = player_color;
    vec2 p = 2 * (frag_texcoord - vec2(0.5));
    float radius = sqrt(dot(p, p));
    if (radius > 1) discard;
    vec3 normal = vec3(p.x, p.y, sqrt(1 - radius * radius));
    // some arbitrary light position, maybe use a uniform?
    vec3 lightdir = normalize(vec3(0.25, -0.25, 1.0));
    if (2 * capture - 1.0 > -p.y) {
      color = cap_color;
    }
    vec4 diffuse = color * max(0, dot(normal, lightdir));
    // specular component
    float shininess = 20.f;
    vec3 eyeVec = normalize(-normal);  // frag/lightpos are in eye space
    vec3 lightVec = normalize(reflect(lightdir, normal));
    float specPower = pow(max(dot(eyeVec, lightVec), 0.0), shininess);

    float red_alpha = 0;
    if (radius > 0.8 && danger) {
      red_alpha = (radius - 0.8) * 5 * sin(3 * t);
    }

    gl_FragColor = color + diffuse + specPower * vec4(1.0);
}
