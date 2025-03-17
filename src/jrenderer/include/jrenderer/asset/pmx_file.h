#pragma once

#include <string>
#include "Pmx.h"
#include "jrenderer/asset/convert.hpp"
#include "jrenderer/mesh.h"

namespace jre
{
    class PmxFile
    {
    public:
        PmxFile(const std::string &file_name)
        {
            std::ifstream stream = std::ifstream(file_name, std::ios_base::binary);
            m_model.Read(&stream);
        }
        ~PmxFile() = default;

        const pmx::PmxModel &model() const { return m_model; }

    private:
        pmx::PmxModel m_model;
    };

    template <>
    inline Vertex convert_to(const pmx::PmxVertex &vertex)
    {
        return Vertex(glm::vec3(vertex.positon[0], vertex.positon[1], vertex.positon[2]),
                      glm::vec3(vertex.normal[0], vertex.normal[1], vertex.normal[2]),
                      glm::vec2(vertex.uv[0], vertex.uv[1]));
    }

    template <typename VertexType, typename IndexType>
    class PmxMeshBuilder
    {
    public:
        std::vector<VertexType> vertices;
        std::vector<IndexType> indices;
        std::vector<SubMesh> sub_meshes;
        DeviceMeshBuilder<VertexType, IndexType> mesh_builder;
        PmxMeshBuilder(
            vk::SharedDevice device,
            vk::PhysicalDevice physical_device,
            vk::CommandBuffer command_buffer,
            vk::Queue transfer_queue,
            const std::string &file_name)
            : PmxMeshBuilder(
                  device,
                  physical_device,
                  command_buffer,
                  transfer_queue,
                  PmxFile(file_name))
        {
        }

        PmxMeshBuilder(
            vk::SharedDevice device,
            vk::PhysicalDevice physical_device,
            vk::CommandBuffer command_buffer,
            vk::Queue transfer_queue,
            const PmxFile &pmx_file)
            : PmxMeshBuilder(
                  device,
                  physical_device,
                  command_buffer,
                  transfer_queue,
                  build_mesh_data(pmx_file.model()))
        {
        }

        PmxMeshBuilder(
            vk::SharedDevice device,
            vk::PhysicalDevice physical_device,
            vk::CommandBuffer command_buffer,
            vk::Queue transfer_queue,
            std::tuple<std::vector<VertexType>, std::vector<IndexType>, std::vector<SubMesh>> mesh_data)
            : vertices(std::move(std::get<0>(mesh_data))),
              indices(std::move(std::get<1>(mesh_data))),
              sub_meshes(std::move(std::get<2>(mesh_data))),
              mesh_builder(device,
                           physical_device,
                           command_buffer,
                           transfer_queue,
                           vertices,
                           indices)

        {
        }

        Mesh build()
        {
            Mesh mesh = mesh_builder.build();
            mesh.sub_meshes = std::move(sub_meshes);
            return mesh;
        }

        std::shared_ptr<Mesh> build_shared()
        {
            return std::make_shared<Mesh>(std::move(build()));
        }

        static std::tuple<std::vector<VertexType>, std::vector<IndexType>, std::vector<SubMesh>> build_mesh_data(const pmx::PmxModel &model)
        {
            std::vector<VertexType> vertices;
            std::vector<IndexType> indices;
            std::vector<SubMesh> sub_meshes;
            // vertex
            vertices.reserve(model.vertex_count);
            for (int i = 0; i < model.vertex_count; ++i)
            {
                const pmx::PmxVertex &vertex = model.vertices[i];
                vertices.push_back(convert_to<VertexType>(vertex));
            }

            // index
            indices.reserve(model.index_count);
            indices.assign(model.indices.get(), model.indices.get() + model.index_count);

            // sub mesh
            sub_meshes.reserve(model.material_count);
            uint32_t index_offset = 0;
            for (size_t i = 0; i < model.material_count; ++i)
            {
                const pmx::PmxMaterial &material = model.materials[i];
                sub_meshes.push_back(SubMesh(0, index_offset, material.index_count));
                index_offset += material.index_count;
            }
            return {std::move(vertices), std::move(indices), std::move(sub_meshes)};
        }

    private:
    };

}