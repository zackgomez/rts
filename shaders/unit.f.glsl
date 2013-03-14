#version 120

// Light properties
uniform vec3 lightPos; // in view space
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;

// material properties
uniform vec3 baseColor;
uniform float shininess;

varying vec3 fragpos;
varying vec3 fragnorm;
varying vec2 fragtc;

void main()
{
  vec3 normal = normalize(fragnorm);

  vec3 lightdir = normalize(lightPos - fragpos);
  float ndotl = max(dot(normal, lightdir), 0.0);

  // ambient component
  vec3 color = ambientColor;

  // diffuse component
  color += clamp(ndotl, 0, 1) * diffuseColor;
  color *= baseColor;

  // specular component
  if (shininess != 0.0) {
    vec3 eyeVec = normalize(-fragpos);  // frag/lightpos are in eye space
    vec3 lightVec = normalize(reflect(-lightdir, normal));
    float specPower = pow(max(dot(eyeVec, lightVec), 0.0), shininess);
    color += specPower * specularColor;
  }

  gl_FragColor = vec4(color, 1.f);
}
