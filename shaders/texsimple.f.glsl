#version 120

uniform sampler2D tex;
varying vec2 frag_texcoord;


void main()
{
    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);

    gl_FragData[0] = texture2D(tex, tc);
    gl_FragData[1] = vec4(0.0f);
}
