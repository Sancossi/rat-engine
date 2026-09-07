$input a_position, a_texcoord0, a_texcoord1
#include <bgfx_shader.sh>
uniform vec4 u_contour;
void main()
{
    vec4 p = mul(u_modelViewProj, vec4(a_position, 1.0));
    vec4 q = mul(u_modelViewProj, vec4(a_texcoord0, 1.0));
    vec2 direction = (q.xy / q.w - p.xy / p.w) / u_contour.xy;
    float extent = max(length(direction), 0.001);
    vec2 perpendicular = vec2(-direction.y, direction.x) / extent;
    p.xy += perpendicular * a_texcoord1.x * u_contour.xy * u_contour.z * p.w;
    p.z -= u_contour.w * p.w;
    gl_Position = p;
}
