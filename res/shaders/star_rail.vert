
#version 450

layout(location = 0) in vec3 in_position_os;
layout(location = 1) in vec3 in_normal_os;
layout(location = 2) in vec2 in_tex_coord;

struct MVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(set = 1, binding = 0) uniform UniformPerObject {
    MVP mvp;
    vec4 debug_control;
} ubo_per_obj;

// Vectors
// Each of the scalar types, including booleans, have 2, 3, and 4-component vector equivalents. The n digit below can be 2, 3, or 4:
// bvecn: a vector of booleans
// ivecn: a vector of signed integers
// uvecn: a vector of unsigned integers
// vecn: a vector of single-precision floating-point numbers
// dvecn: a vector of double-precision floating-point numbers

// 如果出现dvec  location会占两个，是multiple slots。那如果是bvec 呢？可能不会少因为至少1
// layout(location = 0) in dvec3 inPosition;
// layout(location = 2) in vec3 inColor;

// layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec2 out_tex_coord;
layout(location = 1) out vec3 out_normal_ws;

// 
vec3 transform_direction_os2ws(vec3 normal_os, mat4 model)
{
    return mat3(model) * normal_os;  // assume_uniform_scaling
}

void main() {
    gl_Position = ubo_per_obj.mvp.proj * ubo_per_obj.mvp.view * ubo_per_obj.mvp.model * vec4(in_position_os, 1.0);
    out_tex_coord = in_tex_coord;
    out_normal_ws = normalize(transform_direction_os2ws(in_normal_os, ubo_per_obj.mvp.model));
}