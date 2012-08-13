#version 120

attribute vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec4 texcoord; // u0,v0, u1,v1

varying vec2 frag_texcoord;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    vec2 tc0 = texcoord.xy;
    vec2 tc1 = texcoord.zw;
    vec2 size = tc1 - tc0;
    frag_texcoord = vec2(position.x, position.y) + vec2(0.5f); //[0,1]
    frag_texcoord *= size; // [0, size]
    frag_texcoord += tc0; // [tc0, tc1]
}
