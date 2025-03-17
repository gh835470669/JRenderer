#pragma once

#include "jrenderer/texture.h"
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/pipeline.h"
#include "jrenderer/utils/diff_trigger.hpp"
#include <any>

namespace jre
{
    class MaterialInstance;
    class Material
    {
    public:
        SharedRenderPipeline render_pipeline;
        vk::SharedDescriptorSetLayout descriptor_set_layout;
        vk::SharedDescriptorPool descriptor_pool;

        MaterialInstance create_instance(uint32_t frame_count = 1);
    };

    struct RenderMaterialData
    {
        vk::Pipeline pipeline;
        vk::PipelineLayout pipeline_layout;
        vk::DescriptorSet descriptor_set;
    };

    class IMaterialInstance
    {
    public:
        virtual ~IMaterialInstance() = default;
        virtual RenderMaterialData get_render_data(uint32_t cur_frame) = 0;
        const RenderMaterialData get_render_data(uint32_t cur_frame) const { return const_cast<IMaterialInstance *>(this)->get_render_data(cur_frame); }
        virtual void update_descriptor_set_data(uint32_t cur_frame) = 0;
    };

    class MaterialInstance : public IMaterialInstance
    {
    public:
        Material material;
        std::vector<vk::SharedDescriptorSet> descriptor_sets;

        MaterialInstance(Material material, std::vector<vk::SharedDescriptorSet> descriptor_sets) : material(material), descriptor_sets(descriptor_sets) {}
        void update_descriptor_set_data(uint32_t cur_frame) override {}
        RenderMaterialData get_render_data(uint32_t cur_frame) override
        {
            return {
                material.render_pipeline->pipeline.get(),
                material.render_pipeline->pipeline_layout.get(),
                descriptor_sets[cur_frame].get()};
        }
    };

    class MaterialBuilder
    {
    public:
        struct ShaderCreateInfo
        {
            std::string path;
            SpecializationConstants constants = {};
            std::string entry = "main";
        };

        RenderPipelineResources &render_pipeline_resources;
        PipelineLayoutBuilder pipeline_layout_builder;
        PipelineBuilder pipeline_builder;
        vk::SharedDevice device;
        vk::PhysicalDevice physical_device;
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        uint32_t descriptor_set_count = 100;
        ShaderCreateInfo vertex_shader_info;
        ShaderCreateInfo fragment_shader_info;
        std::unordered_map<std::string, vk::SharedShaderModule> *shader_cache;

        MaterialBuilder(RenderPipelineResources &render_pipeline_resources,
                        PipelineLayoutBuilder pipeline_layout_builder,
                        PipelineBuilder pipeline_builder,
                        vk::SharedDevice device,
                        vk::PhysicalDevice physical_device,
                        vk::CommandBuffer command_buffer,
                        vk::Queue transfer_queue)
            : render_pipeline_resources(render_pipeline_resources),
              pipeline_layout_builder(pipeline_layout_builder),
              pipeline_builder(pipeline_builder),
              device(device),
              physical_device(physical_device),
              command_buffer(command_buffer),
              transfer_queue(transfer_queue) {}

        Material build();
    };
}