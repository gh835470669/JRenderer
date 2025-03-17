#include "glsl/core/lighting.glsl"
#include "space_transform.glsl"
#include "common_data.glsl"

// struct UniformPerFrame
// {
// };

#if defined(VERTEX) || defined(FRAGMENT)
layout(set = set_scene, binding = 0) uniform UniformPerScene
{
    Light main_light;
    CameraTransform camera_trans;
} render_set;
#endif

#ifdef VERTEX
layout(set = set_object, binding = 0) uniform UniformPerObject
{
    ModelTransform model_trans;
} obj;
#endif

// struct UniformPerMaterial
// {

// };