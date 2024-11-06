#pragma once

#include "jrenderer/buffer.h"
#include "jmath.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/command_buffer.h"
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>

namespace jre
{
    class IVertex
    {
    };

    class PipelineVertexInputState
    {
    private:
        std::vector<vk::VertexInputBindingDescription> m_binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> m_attribute_descriptions;

    public:
        PipelineVertexInputState(std::vector<vk::VertexInputBindingDescription> binding_descriptions, std::vector<vk::VertexInputAttributeDescription> attribute_descriptions)
            : m_binding_descriptions(std::move(binding_descriptions)), m_attribute_descriptions(std::move(attribute_descriptions)) {}

        const std::vector<vk::VertexInputBindingDescription> &binding_descriptions() const { return m_binding_descriptions; }
        const std::vector<vk::VertexInputAttributeDescription> &attribute_descriptions() const { return m_attribute_descriptions; }

        vk::PipelineVertexInputStateCreateInfo pipeline_vertex_input_state() const;
    };

    class Vertex : public IVertex
    {
    public:
        jmath::vec3 pos;
        jmath::vec3 color;
        jmath::vec2 tex_coord;

        bool operator==(const Vertex &other) const
        {
            return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
        }

        static vk::VertexInputBindingDescription get_binding_description();
        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions();
        static PipelineVertexInputState get_pipeline_vertex_input_state();
    };

    template <typename VertexType = Vertex, typename IndexType = uint32_t, std::enable_if_t<std::is_base_of<IVertex, VertexType>::value, int> = 0>
    struct MeshData
    {
        std::vector<VertexType> vertices;
        std::vector<IndexType> indices;
    };

    template <typename VertexType = Vertex, typename IndexType = uint32_t, std::enable_if_t<std::is_base_of<IVertex, VertexType>::value, int> = 0>
    class Mesh
    {
    private:
        VertexBuffer<VertexType> m_vertex_buffer;
        IndexBuffer<IndexType> m_index_buffer;

    public:
        Mesh(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, const MeshData<VertexType> &mesh_data)
            : m_vertex_buffer(device, command_buffer, mesh_data.vertices), m_index_buffer(device, command_buffer, mesh_data.indices) {}

        const VertexBuffer<VertexType> &vertex_buffer() const { return m_vertex_buffer; }
        const IndexBuffer<IndexType> &index_buffer() const { return m_index_buffer; }
    };

    template <typename VertexType = Vertex, typename IndexType = uint32_t, std::enable_if_t<std::is_base_of<IVertex, VertexType>::value, int> = 0>
    class IMeshLoader
    {
    public:
        virtual MeshData<VertexType, IndexType> load(const std::string &name) = 0;
    };

    class LogicalDevice;

    template <typename VertexType = Vertex, typename IndexType = uint32_t, std::enable_if_t<std::is_base_of<IVertex, VertexType>::value, int> = 0>
    class MeshResources
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        std::unique_ptr<const CommandBuffer> m_default_command_buffer;
        Resources<std::string, const Mesh<VertexType, IndexType>> m_meshes;

    public:
        MeshResources(gsl::not_null<const LogicalDevice *> device, std::unique_ptr<const CommandBuffer> default_command_buffer) : m_device(device), m_default_command_buffer(std::move(default_command_buffer)) {}

        std::shared_ptr<const Mesh<VertexType, IndexType>> get_mesh(const std::string &name, IMeshLoader<VertexType, IndexType> &&loader, const CommandBuffer *command_buffer = nullptr)
        {
            return std::make_shared<const Mesh<VertexType, IndexType>>(m_device, command_buffer ? *command_buffer : *m_default_command_buffer, loader.load(name));
        }

        std::shared_ptr<const Mesh<VertexType, IndexType>> make_mesh(const std::string &name, std::vector<VertexType> vertices, std::vector<IndexType> indices, const CommandBuffer *command_buffer = nullptr)
        {
            return m_meshes.create(name, vertices, indices);
        }
    };

    using DefaultMesh = Mesh<Vertex, uint32_t>;
}

template <>
struct std::hash<jre::Vertex>
{
    size_t operator()(jre::Vertex const &vertex) const
    {
        return ((hash<jmath::vec3>()(vertex.pos) ^
                 (hash<jmath::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<jmath::vec2>()(vertex.tex_coord) << 1);
    }
};