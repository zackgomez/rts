#version 120

uniform sampler2D texture;
uniform vec4 color;
varying vec2 frag_texcoord;

void main()
{
    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);
    float visible = texture2D(texture, tc).a;
    visible = 0.5 + 0.5f * smoothstep(0, 1, visible);

    gl_FragColor = vec4(visible * color.rgb, color.a);
}
