#pragma once

#include "jrenderer/render_set.h"
#include "jrenderer/texture.h"
#include "Pmx.h"
#include "jrenderer/asset/convert.hpp"
#include "jrenderer/asset/pmx_file.h"
#include "jrenderer/material.h"
#include "jrenderer/light.h"
#include "jrenderer/specilization_constant.hpp"

namespace jre
{
    enum class ModelPart
    {
        Body,
        Hair,
        Face,
        PartNum
    };

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

    struct PmxUniformPerRenderSet
    {
        UniformLight main_light;
        glm::vec3 debug_control;
    };

    struct PmxUniformPerObject
    {
        UniformMVP mvp;
        glm::vec4 debug_control;
    };

    struct PmxMaterial
    {
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::unique_ptr<DescriptorSet> descriptor_set;
        std::unique_ptr<DescriptorSet> outline_descriptor_set;
    };

    class PmxModel
    {
    public:
        glm::mat4 model_matrix = glm::identity<glm::mat4>();
        std::shared_ptr<const PmxMesh> mesh;
        std::vector<PmxMaterial> sub_mesh_materials;
        std::vector<ModelPart> model_parts;
        std::unique_ptr<UniformBuffer<PmxUniformPerObject>> uniform_buffer;

        glm::mat4 get_model_matrix() const { return model_matrix; }
    };

    class StarRailCharRenderSet
    {
    public:
        std::shared_ptr<GraphicsPipeline> graphics_pipeline_body;
        std::shared_ptr<GraphicsPipeline> graphics_pipeline_hair;
        std::shared_ptr<GraphicsPipeline> graphics_pipeline_face;
        std::shared_ptr<GraphicsPipeline> graphics_pipeline_outline;      // backface
        std::shared_ptr<GraphicsPipeline> graphics_pipeline_outline_face; // backface

        std::vector<std::reference_wrapper<PmxModel>> render_objects;
        glm::mat4 view_matrix;
        DirectionalLight main_light;
        UniformBuffer<PmxUniformPerRenderSet> ubo;
        std::unique_ptr<DescriptorSet> descriptor_set;

        StarRailCharRenderSet(gsl::not_null<const LogicalDevice *> device) : graphics_pipeline_body(nullptr),
                                                                             graphics_pipeline_hair(nullptr),
                                                                             graphics_pipeline_face(nullptr),
                                                                             render_objects(),
                                                                             view_matrix(glm::identity<glm::mat4>()),
                                                                             ubo(device),
                                                                             descriptor_set(nullptr)
        {
        }

        std::shared_ptr<GraphicsPipeline> get_graphics_pipeline(ModelPart model_part) const
        {
            switch (model_part)
            {
            case ModelPart::Body:
                return graphics_pipeline_body;
            case ModelPart::Face:
                return graphics_pipeline_face;
            case ModelPart::Hair:
                return graphics_pipeline_hair;
            default:
                throw std::runtime_error("Unknown model part");
            }
        }
    };

    class StarRailCharRenderSetRenderer : public IRenderSetRenderer
    {
    public:
        const StarRailCharRenderSet *render_set;
        std::function<vk::Viewport(const vk::Extent2D &)> func_get_viewport;
        std::function<vk::Rect2D(const vk::Extent2D &)> func_get_scissor;
        void draw(const Graphics &graphics, const CommandBuffer &command_buffer) override;
        void update_ubo_per_obj(const SwapChain &swap_chian, PmxModel &render_obj) const;
        void update_ubo_per_render_set();
    };
}