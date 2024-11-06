#include "jrenderer/mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace jre
{
    vk::PipelineVertexInputStateCreateInfo PipelineVertexInputState::pipeline_vertex_input_state() const
    {
        vk::PipelineVertexInputStateCreateInfo vertex_input_info;
        vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(m_binding_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = m_binding_descriptions.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attribute_descriptions.size());
        vertex_input_info.pVertexAttributeDescriptions = m_attribute_descriptions.data();
        return vertex_input_info;
    }

    vk::VertexInputBindingDescription Vertex::get_binding_description()
    {
        vk::VertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = vk::VertexInputRate::eVertex;
        return binding_description;
    }

    std::vector<vk::VertexInputAttributeDescription> Vertex::get_attribute_descriptions()
    {
        std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(3);
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
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

    PipelineVertexInputState Vertex::get_pipeline_vertex_input_state()
    {
        return PipelineVertexInputState({Vertex::get_binding_description()}, Vertex::get_attribute_descriptions());
    }

}
