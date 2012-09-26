#version 120

attribute vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec2 mapSize;

varying vec2 fragPos;

void main()
{
    fragPos = vec2(position.x, -position.y) + 0.5f;
    gl_Position = projectionMatrix * modelViewMatrix * position;
}
