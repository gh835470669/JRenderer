
#version 450

layout(location = 0) in vec3 in_position_os;
layout(location = 1) in vec3 in_normal_os;

struct MVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(set = 0, binding = 0) uniform UniformPerObject {
    MVP mvp;
    vec4 debug_control;
} ubo_per_obj;

const float model_scale = 1.0f;
const float outline_width = 0.01f;
const float outline_zoffert = 0.0f;

layout (constant_id = 0) const uint c_model_part = 0U; // 0 是身体，1是头发，2是脸
const bool c_is_body = (c_model_part == 0U);
const bool c_is_hair = (c_model_part == 1U);
const bool c_is_face = (c_model_part == 2U);

layout(location = 0) out vec4 out_color;


vec3 transform_direction_os2ws(vec3 direction_os, mat4 model)
{
    return normalize(mat3(model) * direction_os);  // assume_uniform_scaling
}

vec3 transform_direction_ws2vs(vec3 direction_ws, mat4 view)
{
    // view matrix: https://stackoverflow.com/questions/13832505/world-space-to-camera-space
    return normalize((view * vec4(direction_ws, 0.0f)).xyz);  // assume_uniform_scaling
}

void main() {
    // TODO: depth dependent, fov dependent, screen apsect dependent, sub mesh depedent width
    vec4 position_vs = ubo_per_obj.mvp.view * ubo_per_obj.mvp.model * vec4(in_position_os, 1.0);
    vec3 normal_ws = transform_direction_os2ws(in_normal_os, ubo_per_obj.mvp.model);
    vec3 normal_vs = transform_direction_ws2vs(normal_ws, ubo_per_obj.mvp.view);
    normal_vs = normalize(vec3(normal_vs.xy, 0.0f)); // 拍扁，无深度区别
    // float fix_scale = -position_vs.z * 0.025;
    float outline_width_adjust = outline_width * model_scale;
    if (!c_is_face)
        position_vs += vec4(normal_vs, 0.0f) * outline_width_adjust;
    gl_Position = ubo_per_obj.mvp.proj * position_vs;
    out_color = vec4(vec3(0.0f), 1);
}