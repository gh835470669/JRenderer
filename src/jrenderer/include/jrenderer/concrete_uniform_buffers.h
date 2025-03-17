#pragma once

#include "jmath.h"
#include "jrenderer/light.h"
#include <glm/ext/matrix_transform.hpp>

namespace jre
{
    struct UniformModelTransform
    {
        glm::mat4 model = glm::identity<glm::mat4>();
        glm::mat4 model_view;
        glm::mat4 model_view_proj;
    };

    struct UniformCamera
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 view_proj;
    };

    // UBO : Uniform Buffer Object
    struct UniformPerRenderSet
    {
        UniformLight main_light;
        UniformCamera camera_trans;
    };

    struct UniformScene
    {
        UniformLight main_light;
        UniformCamera camera_trans;
    };

    struct UniformPerObject
    {
        UniformModelTransform mvp;
    };

    enum class UniformBufferSetIndex
    {
        // PerFrame,
        PerRenderSet,
        PerObject,
        PerMaterial
    };
}