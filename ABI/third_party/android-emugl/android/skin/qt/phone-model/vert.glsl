attribute vec3 vtx_pos;
attribute vec3 vtx_normal;
attribute vec2 vtx_uv;

varying vec3 varying_pos;
varying vec3 varying_normal;
varying vec2 varying_uv;

uniform mat4 model_view_inverse_transpose;
uniform mat4 model_view;
uniform mat4 model_view_projection;
void main() {
    // Blender seems to export UV coordinates in a weird
    // way (V is inverted compared to how OpenGL sees it).
    // It's easier to fix it here in the shader...
    varying_uv = vec2(vtx_uv.x, 1.0 - vtx_uv.y);
    vec4 normal = vec4(vtx_normal.x, vtx_normal.y, vtx_normal.z, 0);

    // Apply transformation to the normal.
    varying_normal = (model_view_inverse_transpose * normal).xyz;

    // Apply the modelview transform to the vertex to compute the
    // final position.
    vec4 pos4 = vec4(vtx_pos.x, vtx_pos.y, vtx_pos.z, 1);
    vec4 vpos4 = model_view * pos4;
    varying_pos = vpos4.xyz/vpos4.w;
    gl_Position = model_view_projection * pos4;
}