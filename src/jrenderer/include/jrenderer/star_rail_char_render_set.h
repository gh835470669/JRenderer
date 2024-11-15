#pragma once

#include "jrenderer/render_set.h"
#include "jrenderer/texture.h"
#include "Pmx.h"
#include "jrenderer/asset/convert.hpp"
#include "jrenderer/asset/pmx_file.h"
#include "jrenderer/material.h"

namespace jre
{
    class PmxVertex
    {
    public:
        jmath::vec3 position;
        jmath::vec3 normal;
        jmath::vec2 uv;

        static vk::VertexInputBindingDescription get_binding_description(uint32_t binding);
        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding);
        static PipelineVertexInputState get_pipeline_vertex_input_state(uint32_t binding);
    };

    class PmxMeshData
    {
    public:
        using SubMeshRegionOffset = uint32_t;
        using SubMeshRegionCount = uint32_t;
        using SubMeshRegion = std::pair<SubMeshRegionOffset, SubMeshRegionCount>;

        std::vector<PmxVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SubMeshRegion> sub_mesh_indices;
    };

    class PmxSubMesh;
    class PmxMesh
    {
    public:
        PmxMesh(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, const PmxMeshData &mesh_data);
        ~PmxMesh() = default;

        const VertexBuffer<PmxVertex> &vertex_buffer() const { return m_vertex_buffer; }
        const IndexBuffer<uint32_t> &index_buffer() const { return m_index_buffer; }
        const std::vector<PmxSubMesh> &sub_meshes() const { return m_sub_meshes; }

    private:
        VertexBuffer<PmxVertex> m_vertex_buffer;
        IndexBuffer<uint32_t> m_index_buffer;
        std::vector<PmxSubMesh> m_sub_meshes;
    };

    class PmxSubMesh
    {
    public:
        PmxSubMesh(const PmxMesh &mesh, uint32_t offset, uint32_t count);
        ~PmxSubMesh() = default;

        const IndexBufferSpan<uint32_t> &index_buffer() const { return m_index_buffer; }

    private:
        IndexBufferSpan<uint32_t> m_index_buffer;
    };

    class PmxModel
    {
    public:
        glm::mat4 model_matrix = glm::identity<glm::mat4>();
        std::shared_ptr<const PmxMesh> mesh;
        std::vector<Material> sub_mesh_materials;

        glm::mat4 get_model_matrix() const { return model_matrix; }
    };

    class StarRailCharRenderSet
    {
    public:
        std::shared_ptr<GraphicsPipeline> graphics_pipeline;
        std::vector<std::reference_wrapper<PmxModel>> render_objects;
        glm::mat4 view_matrix;
    };

    class StarRailCharRenderSetRenderer : public IRenderSetRenderer
    {
    public:
        const StarRailCharRenderSet *render_set;
        std::function<vk::Viewport(const vk::Extent2D &)> func_get_viewport;
        std::function<vk::Rect2D(const vk::Extent2D &)> func_get_scissor;
        void draw(const Graphics &graphics, const CommandBuffer &command_buffer) override;
        void update_uniform_buffer(const SwapChain &swap_chian, PmxModel &render_obj) const;
    };
}