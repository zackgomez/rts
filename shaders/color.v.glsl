#version 330

layout (location = 0) in vec4 position;
//layout (location = 1) in vec4 color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
}
