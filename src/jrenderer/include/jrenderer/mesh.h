#pragma once

#include "jrenderer/buffer.h"
#include "jmath.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/command_buffer.h"
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#include <ranges>

namespace jre
{

    class Vertex
    {
    public:
        jmath::vec3 pos;
        jmath::vec3 normal;
        jmath::vec2 tex_coord;

        Vertex() : pos(0.0f), normal(0.0f), tex_coord(0.0f) {}
        Vertex(jmath::vec3 pos, jmath::vec3 normal, jmath::vec2 tex_coord) : pos(pos), normal(normal), tex_coord(tex_coord) {}

        bool operator==(const Vertex &other) const
        {
            return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
        }
    };

    template <typename VertexType>
    vk::VertexInputBindingDescription get_binding_description(uint32_t binding)
    {
        vk::VertexInputBindingDescription binding_description{};
        binding_description.binding = binding;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = vk::VertexInputRate::eVertex;
        return binding_description;
    }

    template <typename VertexType>
    std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding) { return {}; }

    template <>
    inline std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions<Vertex>(uint32_t binding)
    {
        return {
            {0, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
            {1, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
            {2, binding, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)},
        };
    }

    struct RenderSubMeshData
    {
        uint32_t vertex_offset;
        uint32_t index_offset;
        uint32_t index_count;
    };

    struct RenderMeshData
    {
        std::vector<vk::Buffer> vertexes;
        vk::Buffer index_buffer;
        vk::IndexType index_type;
        std::vector<RenderSubMeshData> sub_meshes;
    };

    class IMesh
    {
    public:
        virtual ~IMesh() = default;
        virtual RenderMeshData get_render_data() = 0;
        const RenderMeshData get_render_data() const { return const_cast<IMesh *>(this)->get_render_data(); }
    };

    class ISubMesh
    {
    public:
        virtual RenderSubMeshData get_render_data() = 0;
        const RenderSubMeshData get_render_data() const { return const_cast<ISubMesh *>(this)->get_render_data(); }
    };

    class SubMesh : public ISubMesh
    {
    public:
        uint32_t vertex_offset;
        uint32_t index_offset;
        uint32_t index_count;

        SubMesh(uint32_t vertex_offset, uint32_t index_offset, uint32_t index_count) : vertex_offset(vertex_offset), index_offset(index_offset), index_count(index_count) {}

        RenderSubMeshData get_render_data() override { return {vertex_offset, index_offset, index_count}; }
    };

    class Mesh : public IMesh
    {
    public:
        DynamicBuffer vertex_buffer;
        DynamicBuffer index_buffer;
        vk::IndexType index_type;
        std::vector<SubMesh> sub_meshes;

        RenderMeshData get_render_data() override
        {
            RenderMeshData mesh_data;
            mesh_data.vertexes = {vertex_buffer.buffer().get()};
            mesh_data.index_buffer = index_buffer.buffer().get();
            mesh_data.index_type = index_type;
            mesh_data.sub_meshes = sub_meshes |
                                   std::views::transform([](SubMesh &sub_mesh)
                                                         { return sub_mesh.get_render_data(); }) |
                                   std::ranges::to<std::vector>();
            return mesh_data;
        }

    private:
    };

    template <typename VertexType, typename IndexType>
    class DeviceMeshBuilder
    {
    public:
        DeviceArrayBufferBuilder<VertexType> vertex_buffer_builder;
        DeviceArrayBufferBuilder<IndexType> index_buffer_builder;
        DeviceMeshBuilder(vk::SharedDevice device,
                          vk::PhysicalDevice physical_device,
                          vk::CommandBuffer command_buffer,
                          vk::Queue transfer_queue,
                          vk::ArrayProxyNoTemporaries<VertexType> vertex_data,
                          vk::ArrayProxyNoTemporaries<IndexType> index_data)
            : vertex_buffer_builder(device, physical_device, command_buffer, transfer_queue, vertex_data),
              index_buffer_builder(device, physical_device, command_buffer, transfer_queue, index_data)
        {
            vertex_buffer_builder.set_usage(vk::BufferUsageFlagBits::eVertexBuffer);
            index_buffer_builder.set_usage(vk::BufferUsageFlagBits::eIndexBuffer);
        }

        Mesh build()
        {
            Mesh mesh;
            mesh.vertex_buffer = vertex_buffer_builder.build();
            mesh.index_buffer = index_buffer_builder.build();
            mesh.index_type = vk::IndexTypeValue<IndexType>::value;
            return mesh;
        }
    };

    template <typename VertexType, typename IndexType>
    class HostMesh : public IMesh
    {
    public:
        HostArrayBuffer<VertexType> vertex_buffer;
        HostArrayBuffer<IndexType> index_buffer;
        std::vector<SubMesh> sub_meshes;

        RenderMeshData get_render_data() override
        {
            RenderMeshData mesh_data;
            mesh_data.vertexes = {vertex_buffer.vk_buffer()};
            mesh_data.index_buffer = index_buffer.vk_buffer();
            mesh_data.index_type = vk::IndexTypeValue<IndexType>::value;
            mesh_data.sub_meshes = sub_meshes |
                                   std::views::transform([](SubMesh &sub_mesh)
                                                         { return sub_mesh.get_render_data(); }) |
                                   std::ranges::to<std::vector>();
            return mesh_data;
        }
    };

}