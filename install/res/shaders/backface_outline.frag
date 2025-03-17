#version 450

#define FRAGMENT
#include "common_inputs.glsl"
#include "star_rail_outline_inputs.glsl"

void main() {
    out_color = vec4((texture(main_tex_sampler, vs_out.tex_coord).rgb * (1.0f - props.outline.factor_of_color)) + (props.outline.color * props.outline.factor_of_color), 1.0f);
}