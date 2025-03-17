#version 450

#define FRAGMENT
#include "common_inputs.glsl"
#include "star_rail_inputs.glsl"


float get_half_lambert_ao(float half_lambert, float lightmap_ao, float shadow_ramp)
{
    float half_lambert_ao   = min(1.0f, dot(half_lambert.xx, 2.0f * lightmap_ao.xx));
    half_lambert_ao = smoothstep(0.5f, 1.0f, half_lambert_ao);  // [0, 1]
    half_lambert_ao = max(0.001f, half_lambert_ao) * 0.85f + 0.15f;
    half_lambert_ao = (half_lambert_ao > shadow_ramp) ? 0.99f : half_lambert_ao;
    return half_lambert_ao;
}

// control
const bool k_use_specular = !k_is_face;
const bool k_use_lightmap = !k_is_face;


void main() {
    // vectors
	vec3 light_dir_ws = get_light_dir_norm(render_set.main_light);  // world pos to light
    vec3 halfway_dir_ws = normalize(vs_out.view_dir_ws + light_dir_ws);
	float ndotl         = dot(vs_out.normal_ws, light_dir_ws);  // [-1, 1]
    float ndoth         = dot(vs_out.normal_ws, halfway_dir_ws);
    float ndotv         = dot(vs_out.normal_ws, vs_out.view_dir_ws);
    float half_lambert  = 0.5 + 0.5 * ndotl;

    // colors
    vec3 base_color = texture(main_tex_sampler, vs_out.tex_coord).rgb;
    vec3 light_color = get_light_color(render_set.main_light);

    // light map
    vec4 light_map = {1.0f, 1.0f, 1.0f, 0.0f};
    if (k_use_lightmap)
    {
        light_map = texture(light_map_sampler, vs_out.tex_coord);
    }
    float lightmap_ao = light_map.g;
    float lightmap_specular_thresh = light_map.b;
    int lightmap_region = int(floor(8 * light_map.a));  // [0, 1] -> [0, 1, 2, ... , 8]

    // diffuse color
    float shadow_area = get_half_lambert_ao(half_lambert, lightmap_ao, 1.0f);
    float ramp_id = (lightmap_region * 2.0f + 1.0f) * 0.0625f;  // [0, 1, 2, ... , 8] -> [0.0625, 0.125, ... , 1]
    vec2 ramp_uv = {shadow_area, ramp_id};
    vec3 ramp_cool = texture(cool_ramp_sampler, ramp_uv).rgb;
    vec3 ramp_warm = texture(warm_ramp_sampler, ramp_uv).rgb;
    float ramp_cool_or_warm = 1.0f;
    vec3 ramp_color = mix(ramp_cool, ramp_warm, ramp_cool_or_warm);
    vec3 diffuse_color = base_color * ramp_color * light_color;

    // specular color
    vec3 specular_color = vec3(0.0f);
    if (k_use_specular)
    {
        float specular_shininess = props.specular_shininess[lightmap_region];
        float specular_roughness = props.specular_roughness[lightmap_region];

        // https://github.com/stalomeow/StarRailNPRShader
        // float blinn_phong_specular = pow(max(ndoth, 0.01f), 10.0f);
        // float threshold = 1.03 - light_map.b;
        // float roughness = props.debug_control.x;
        // float specular = smoothstep(threshold - roughness, threshold + roughness, blinn_phong_specular);
        // specular *= light_map.r;
        // vec3 specular_color = specular * light_color;

        // https://li-kira.github.io/2023/11/13/ToonShader/
        // 各向异性高光
        // float aniso_fresnel = pow((1.0 - clamp(ndotv, 0.0f, 1.0f)), specular_shininess);
        // float aniso = clamp(1 - aniso_fresnel, 0.0f, 1.0f) * lightmap_specular_thresh * half_lambert * 0.3;
        // specular_color = vec3(aniso);

        // https://github.com/Hoyotoon/HoyoToon
        float specular = pow(max(ndoth, 0.01f), specular_shininess);
        float specular_thresh = 1.0f - light_map.b;
        float rough_thresh = specular_thresh - specular_roughness;
        specular_thresh = (specular_roughness + specular_thresh) - rough_thresh;
        specular = shadow_area * specular - rough_thresh; 
        specular_thresh = clamp((1.0f / specular_thresh) * specular, 0.0f, 1.0f);
        specular = (specular_thresh * - 2.0f + 3.0f) * pow(specular_thresh, 2.0f);
        specular *= 0.35f;
        specular_color = specular * light_color;
    }

    out_color = vec4(specular_color + diffuse_color, 1.0f);
    if (debug.show_material_region)
        out_color = vec4(vec3(light_map.a), 1.0f);
}
