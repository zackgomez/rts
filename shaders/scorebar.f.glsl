#version 120

uniform sampler2D tex;
uniform float t;
uniform vec4 color1;
uniform vec4 color2;
uniform float factor;
uniform vec2 vp_count;
varying vec2 frag_texcoord;

void main()
{
    vec2 p = frag_texcoord;
    bool p1 = p.x < factor;
    vec4 color;
    vec4 texcolor;
    if (p1) {
      float rate = max(vp_count.x / 2, 0.1);
      color = color1;
      vec2 fragcoord_t = frag_texcoord - rate * vec2(t, 0);
      fragcoord_t.x = mod(fragcoord_t.x, 1.0);
      texcolor = texture2D(tex, fragcoord_t);
    } else {
      float rate = max(vp_count.y / 2, 0.1);
      color = color2;
      vec2 fragcoord_t = frag_texcoord + rate * vec2(t, 0);
      fragcoord_t.x = mod(fragcoord_t.x, 1.0);
      texcolor = texture2D(tex, fragcoord_t);
    }
    float glow_falloff = 3 * (sin(max(vp_count.x, vp_count.y) * t) + 8);
    vec4 glow = max((1 - abs(factor - p.x) * glow_falloff), 0) * vec4(1, 1, 1, 1);

    gl_FragColor = color + (factor - abs(factor - p.x)) * 2 * texcolor + glow;
}
