
#version 450

#define VERTEX
#include "common_inputs.glsl"
#include "star_rail_inputs.glsl"

void main() {
    vec4 position_ws = trans_point_os2ws(obj.model_trans.model, vec4(in_position_os, 1.0));
    gl_Position = trans_point_ws2cs(render_set.camera_trans.view_proj, position_ws);
    vs_out.tex_coord = in_tex_coord;
    vs_out.normal_ws = trans_dir_os2ws_norm(obj.model_trans.model, in_normal_os);
    vec4 camera_pos_ws = get_camera_pos_ws(render_set.camera_trans.view);
    vs_out.view_dir_ws = normalize((camera_pos_ws - position_ws).xyz);
}