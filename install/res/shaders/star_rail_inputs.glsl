#include "common_data.glsl"
#include "star_rail_common_inputs.glsl"


struct VetextShaderOutput
{
    vec2 tex_coord;
    vec3 normal_ws;
    vec3 view_dir_ws;
};

#ifdef VERTEX  // Vertex Shader

layout(location = 0) in vec3 in_position_os;
layout(location = 1) in vec3 in_normal_os;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out VetextShaderOutput vs_out;

#endif



#ifdef FRAGMENT  // Fragment Shader

MODEL_PART_SPECIAL_CONSTANT_DEFINITION

layout(location = 0) in VetextShaderOutput vs_out;

DEBUG_UNIFORM_DEFINITION

CUSTOM_PROPERTIES_UNIFORM_DEFINITION

layout(set = set_material, binding = k_custom_bind_index + 1) uniform sampler2D main_tex_sampler;  // diffuse texture
layout(set = set_material, binding = k_custom_bind_index + 2) uniform sampler2D light_map_sampler;
layout(set = set_material, binding = k_custom_bind_index + 3) uniform sampler2D cool_ramp_sampler;
layout(set = set_material, binding = k_custom_bind_index + 4) uniform sampler2D warm_ramp_sampler;

layout(location = 0) out vec4 out_color;

#endif