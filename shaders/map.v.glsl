#version 120

attribute vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec2 mapSize;

varying vec4 fragPos;

void main()
{
    fragPos = vec4(mapSize, 1.f, 1.f) * position;
    gl_Position = projectionMatrix * modelViewMatrix * position;
}
