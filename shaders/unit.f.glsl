#version 120

uniform vec4 color;

uniform vec3 lightPos; // in view space

varying vec3 fragpos;
varying vec3 fragnorm;
varying vec2 fragtc;

void main()
{
    vec3 normal = normalize(fragnorm);

    vec3 lightdir = normalize(lightPos - fragpos);
    float ndotl = max(dot(normal, lightdir), 0.0);
    float ambient = 0.3;

    // Some basic diffuse lighting...
    float pow = clamp(ndotl, 0, 1);

    gl_FragColor = pow * color;
    //gl_FragColor = vec4(lightdir, 1);// pow * color;
    //gl_FragColor = vec4(normal, 1);
}
