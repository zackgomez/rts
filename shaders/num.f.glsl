#version 120

uniform sampler2D tex;

varying vec2 frag_texcoord;

uniform vec3 color;


void main()
{
    gl_FragData[0] = vec4(color, 1.f) * texture2D(tex, frag_texcoord);
    gl_FragData[1] = vec4(0.0f);
}
