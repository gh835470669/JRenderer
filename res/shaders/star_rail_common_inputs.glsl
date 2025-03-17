#extension GL_EXT_scalar_block_layout : enable

const int k_material_region_count = 8;
const int k_debug_bind_index = 0;
const int k_custom_bind_index = k_debug_bind_index + 1;

struct Outline
{
    vec3 color;
    float factor_of_color;
    float width;
};

// https://docs.vulkan.org/guide/latest/shader_memory_layout.html
// 让specular_shininess和specular_roughness的一个变量是4字节，而不是16字节

#define DEBUG_UNIFORM_DEFINITION layout(set = set_material, binding = k_debug_bind_index) uniform Debug { \
    vec4 debug_control; \
    bool show_material_region; \
} debug;

#define CUSTOM_PROPERTIES_UNIFORM_DEFINITION layout(std430, set = set_material, binding = k_custom_bind_index) uniform Properties { \
    Outline outline; \
    vec4 specular_colors[k_material_region_count]; \
    float specular_shininess[k_material_region_count]; \
    float specular_roughness[k_material_region_count]; \
} props;



#define MODEL_PART_SPECIAL_CONSTANT_DEFINITION layout (constant_id = 0) const uint k_model_part = 0u; \
const bool k_is_body = (k_model_part == 0u); \
const bool k_is_hair = (k_model_part == 1u); \
const bool k_is_face = (k_model_part == 2u);