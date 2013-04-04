#version 120

uniform sampler2D texture;
uniform vec4 color;
varying vec2 frag_texcoord;

void main()
{
    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);
    float visible = texture2D(texture, tc).a;

    vec4 finalColor = color;
    if (visible == 0.f) {
      finalColor.rgb *= 0.5f;
    }

    gl_FragColor = finalColor;
}
