$input v_texcoord0, v_color0
#include <bgfx_shader.sh>
SAMPLER2D(s_atlas, 0);
void main() {
  vec4 color = texture2D(s_atlas, v_texcoord0) * v_color0;
  if (color.a < 0.5) discard;
  gl_FragColor = vec4(color.rgb, 1.0);
}
