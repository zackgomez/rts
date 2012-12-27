#version 120

uniform sampler2D tex;
uniform vec4 color;
varying vec2 frag_texcoord;


void main()
{
    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);

    vec4 texcol = texture2D(tex, tc);
    if (texcol.a != 0) {
      texcol = color * texcol.a;
    }

    gl_FragColor = texcol;
}
