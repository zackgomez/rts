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
    vec4 diffuse = color * max(0, dot(normal, lightdir));
    // specular component
    float shininess = 20.f;
    vec3 eyeVec = normalize(-normal);  // frag/lightpos are in eye space
    vec3 lightVec = normalize(reflect(lightdir, normal));
    float specPower = pow(max(dot(eyeVec, lightVec), 0.0), shininess);

    gl_FragColor = color + diffuse + specPower * vec4(1.0);
}
