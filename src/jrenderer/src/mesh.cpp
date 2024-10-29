#include "jrenderer/mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <numeric>

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

    MeshData<Vertex> jre::ObjLoader::load(const std::string &name) const
    {
        MeshData<Vertex> mesh_data;

        // 加载模型
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, name.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        // 创建顶点缓冲区
        mesh_data.vertices.reserve(attrib.vertices.size() / 3);
        mesh_data.indices.reserve(std::accumulate(shapes.begin(), shapes.end(), 0, [](uint32_t acc, const tinyobj::shape_t &shape)
                                                  { return acc + static_cast<uint32_t>(shape.mesh.indices.size()); }));

        std::unordered_map<Vertex, uint32_t> unique_vertices{};
        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
                // if (index.normal_index >= 0)
                // {
                //     vertices[index.vertex_index].normal = {
                //         attrib.normals[3 * index.normal_index + 0],
                //         attrib.normals[3 * index.normal_index + 1],
                //         attrib.normals[3 * index.normal_index + 2]};
                // }
                if (index.texcoord_index >= 0)
                {
                    vertex.tex_coord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                }
                vertex.color = {1.0f, 1.0f, 1.0f};
                if (unique_vertices.count(vertex) == 0)
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(mesh_data.vertices.size());
                    mesh_data.vertices.push_back(vertex);
                }
                mesh_data.indices.push_back(unique_vertices[vertex]);
            }
        }
        return mesh_data;
    }

}
