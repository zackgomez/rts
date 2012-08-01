#version 120

attribute vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

uniform vec2 pos;
uniform vec2 size;

varying vec2 frag_texcoord;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;

    frag_texcoord = vec2(position.x, position.y) + vec2(0.5f); // to [0, 1]
    frag_texcoord *= size; // to [0, size]
    frag_texcoord += pos; // to [pos, pos+size]
}
