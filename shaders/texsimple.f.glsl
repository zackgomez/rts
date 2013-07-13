#version 120

uniform sampler2D tex;
varying vec2 frag_texcoord;
uniform vec4 color;


void main()
{
    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);

    gl_FragColor = color * texture2D(tex, tc);
}
