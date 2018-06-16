#version 330
uniform vec4 lumpos;
uniform sampler1D degrade;
in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec4 vsoModPosition;
in vec3 vsoPosition;

out vec4 fragColor;

void main(void) {
  vec3 lum = normalize(vsoModPosition.xyz - lumpos.xyz);
  fragColor = vec4(texture(degrade, (1.0 + vsoPosition.y) / 2.0).rgb * (vec3(0.1) + 0.9 * vec3(1) * dot(normalize(vsoNormal), -lum)), 1.0);
}
