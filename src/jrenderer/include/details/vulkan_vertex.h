#pragma once

#include "jmath.h"
#include <vulkan/vulkan.hpp>

namespace jre
{
    struct Vertex
    {
        jmath::vec3 pos;
        jmath::vec3 color;
        jmath::vec2 tex_coord;

        static std::array<vk::VertexInputBindingDescription, 1> get_binding_descriptions()
        {
            // 可以看的出来，一个vertex buffer 可以有多个binding，一个binding应该是和shader的一次对应。是不是一个vertex buffer可以多次渲染，绑不同的shader
            std::array<vk::VertexInputBindingDescription, 1> binding_descriptions{};
            binding_descriptions[0].binding = 0;
            binding_descriptions[0].stride = sizeof(Vertex);
            // VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
            // VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance     instanced rendering?
            binding_descriptions[0].inputRate = vk::VertexInputRate::eVertex;
            return binding_descriptions;
        }

        static std::array<vk::VertexInputAttributeDescription, 3> get_attribute_descriptions()
        {
            // It is allowed to use more channels than the number of components in the shader, but they will be silently discarded.
            // CPU传的数据可以比shader多，多的会被丢弃。当然一致才是最重要的，少传点数据少点耗能嘛
            // 如果CPU传的数据比shader少，shader会用默认值填充。颜色RGBA默认值是(0, 0, 0, 1)
            // 类型要三者匹配，Vertex成员变量的类型，desctription的类型要和shader的类型一致

            std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions{};
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            // float: VK_FORMAT_R32_SFLOAT
            // vec2: VK_FORMAT_R32G32_SFLOAT
            // vec3: VK_FORMAT_R32G32B32_SFLOAT
            // vec4: VK_FORMAT_R32G32B32A32_SFLOAT
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(Vertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[1].offset = offsetof(Vertex, color);

            attribute_descriptions[2].binding = 0;
            attribute_descriptions[2].location = 2;
            attribute_descriptions[2].format = vk::Format::eR32G32Sfloat;
            attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

            return attribute_descriptions;
        }
    };
}
