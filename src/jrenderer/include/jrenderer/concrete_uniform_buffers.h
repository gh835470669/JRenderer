#pragma once

#include "jmath.h"
#include "jrenderer/light.h"

namespace jre
{
    struct UniformMVP
    {
        jmath::mat4 model;
        jmath::mat4 view;
        jmath::mat4 proj;
    };

    // UBO : Uniform Buffer Object
    struct UniformPerRenderSet
    {
        UniformLight main_light;
    };

    struct UniformPerObject
    {
        UniformMVP mvp;
    };

    enum class UniformBufferSetIndex
    {
        ePerRenderSet = 0,
        ePerObject = 1
    };
}