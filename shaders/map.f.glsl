#version 120

varying vec2 fragPos;
varying vec2 frag_texcoord;

uniform sampler2D texture;
uniform vec4 color;
uniform vec2 gridDim;

float PI = 3.14159265358979323846264;

void main()
{
  vec2 gridPos = fragPos * gridDim;
  float u = min(abs(sin(gridPos.x * PI)), abs(sin(gridPos.y * PI)));
  u = smoothstep(0, 0.1, u);

	float visible = texture2D(texture, frag_texcoord).a;
	if (visible == 0.f) {
		u /= 2;
	}

  gl_FragColor = vec4(u * color.rgb, color.a);
}
