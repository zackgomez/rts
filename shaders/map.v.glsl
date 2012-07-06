#version 330

layout (location = 0) in vec4 position;
//layout (location = 1) in vec4 color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec2 mapSize;

smooth out vec4 fragPos;

void main()
{
    fragPos = glm::vec4(mapSize, 1.f, 1.f) * position;
    gl_Position = projectionMatrix * modelViewMatrix * position;
}
