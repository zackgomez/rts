#version 120

uniform sampler2D tex;
uniform vec4 color;
uniform float minDist, maxDist;
varying vec2 frag_texcoord;


void main()
{
    const float soft_min = 0.5;
    const float soft_max = 1.5;

    vec2 tc = vec2(frag_texcoord.x, frag_texcoord.y);

    float rawDist = texture2D(tex, tc).r;
    float dist = rawDist * (maxDist - minDist) + minDist;
    if (dist > soft_max) {
      discard;
    } else {
      gl_FragColor = vec4(color.rgb, color.a * smoothstep(soft_max, soft_min, dist));
    }
}
