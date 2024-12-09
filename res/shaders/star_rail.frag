#version 450

#include "glsl/core/lighting.glsl"
#include "glsl/core/macros.glsl"

layout (constant_id = 0) const uint c_model_part = 0U; // 0 是身体，1是头发，2是脸
const bool c_is_body = (c_model_part == 0U);
const bool c_is_hair = (c_model_part == 1U);
const bool c_is_face = (c_model_part == 2U);

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) in vec3 in_normal_ws;

layout(binding = 0) uniform UniformPerRenderSet
{
    Light light;
    vec4 debug_control;
} ubo_per_render_set;

layout(set = 1, binding = 1) uniform sampler2D main_tex_sampler;  // diffuse texture
layout(set = 1, binding = 2) uniform sampler2D light_map_sampler;
layout(set = 1, binding = 3) uniform sampler2D cool_ramp_sampler;
layout(set = 1, binding = 4) uniform sampler2D warm_ramp_sampler;

layout(location = 0) out vec4 out_color;


float shadow_rate(float ndotl, float lightmap_ao, float shadow_ramp)
{
    float half_lambert  = ndotl * 0.5f + 0.5f;  // [-1, 1] -> [0, 1]  half lambert
    float half_lambert_ao   = min(1.0f, dot(half_lambert.xx, 2.0f * lightmap_ao.xx));
    half_lambert_ao = smoothstep(0.5f, 1.0f, half_lambert_ao);  // [0, 1]
    
    #ifndef _IS_PASS_LIGHT
        half_lambert_ao = max(0.001f, half_lambert_ao) * 0.85f + 0.15f;
        half_lambert_ao = (half_lambert_ao > shadow_ramp) ? 0.99f : half_lambert_ao;
    #else
        half_lambert_ao = smoothstep(0.5f, 1.0f, half_lambert_ao);
    #endif
    return half_lambert_ao;
}


vec3 get_ramp_diffuse(
    float ndotl,
    float ramp_cool_or_warm,
    vec4 light_map)
{
    float region_id = floor(8 * light_map.a);;  // [0, 1] -> [0, 1, 2, ... , 8]
    float ramp_id = (region_id * 2.0f + 1.0f) * 0.0625f;  // [0, 1, 2, ... , 8] -> [0.0625, 0.125, ... , 1]
    float shadow_area = shadow_rate(ndotl, light_map.g, ramp_cool_or_warm);
    vec2 ramp_uv = {shadow_area, ramp_id};
    vec3 ramp_cool = texture(cool_ramp_sampler, ramp_uv).rgb;
    vec3 ramp_warm = texture(warm_ramp_sampler, ramp_uv).rgb;
    vec3 ramp_color = mix(ramp_cool, ramp_warm, ramp_cool_or_warm);
    return ramp_color;
}


void main() {
    // light info
	vec3 world_to_light = normalize(-ubo_per_render_set.light.direction.xyz);  // world pos to light
	float ndotl         = dot(in_normal_ws, world_to_light);  // [-1, 1]

    // diffuse color
    vec3 base_color = texture(main_tex_sampler, in_tex_coord).rgb;
    vec3 ramp_color = vec3(1.0f);
    vec4 light_map = vec4(1.0f);
    if (!c_is_face)
    {
        light_map = texture(light_map_sampler, in_tex_coord);
        ramp_color = get_ramp_diffuse(ndotl, 1.0f, light_map);
    }
    vec3 light_color = ubo_per_render_set.light.color.rgb * ubo_per_render_set.light.color.w;
    vec3 diffuse_color = base_color * ramp_color * light_color;
    out_color = vec4(diffuse_color, 1.0f);
}
