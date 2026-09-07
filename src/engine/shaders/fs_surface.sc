$input v_color0, v_normal, v_height
#include <bgfx_shader.sh>
uniform vec4 u_surface;
void main()
{
    vec3 n = normalize(v_normal);
    vec3 light = normalize(vec3(-0.45, 0.82, 0.35));
    float ambient = mix(0.22, 0.42, clamp(n.y + 1.0, 0.0, 1.0));
    float lighting = ambient + 0.80 * max(dot(n, light), 0.0);
    float heightCue = 1.0 + clamp(v_height * u_surface.x * 0.025, -0.08, 0.08);
    vec3 albedo = v_color0.rgb * vec3(1.38, 1.41, 1.47);
    vec3 lit = albedo * lighting * heightCue;
    // Preserve semantic material hues without clipping a bright channel.
    lit /= max(1.0, max(max(lit.r, lit.g), lit.b) / 0.92);
    gl_FragColor = vec4(lit, v_color0.a);
}
