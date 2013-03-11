#version 120

attribute vec4 position;
attribute vec4 normal;
attribute vec2 texcoord;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;

uniform vec3 lightPos;

varying vec3 fragpos;
varying vec3 fragnorm;
varying vec2 fragtc;

void main()
{
    fragnorm = vec3(normalMatrix * normal);
    fragtc = vec2(texcoord.x, 1 - texcoord.y);
    fragpos = vec3(modelViewMatrix * position);

    gl_Position = projectionMatrix * modelViewMatrix * position;
}
