
#version 450

#define VERTEX
#include "common_inputs.glsl"
#include "star_rail_outline_inputs.glsl"

void main() {
    // TODO: depth dependent, fov dependent, screen apsect dependent, sub mesh depedent width
    vec4 position_vs = trans_point_os2vs(obj.model_trans.model_view, homo_point(in_position_os));
    vec3 normal_ws = trans_dir_os2ws_norm(obj.model_trans.model, in_normal_os);
    vec3 normal_vs = trans_dir_ws2vs_norm(render_set.camera_trans.view, normal_ws);
    normal_vs = normalize(vec3(normal_vs.xy, 0.0f)); // 拍扁，无深度区别
    float outline_width_adjust = props.outline.width;
    if (!k_is_face)
        position_vs += vec4(normal_vs, 0.0f) * outline_width_adjust;
    gl_Position = trans_point_vs2cs(render_set.camera_trans.proj, position_vs);
    vs_out.tex_coord = in_tex_coord;
}