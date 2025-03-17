#pragma once

#include <array>
#include "vulkan/vulkan_shared.hpp"
#include "jrenderer/material.h"
#include "jrenderer/pipeline.h"

namespace jre
{
    enum class ModelPart
    {
        Body,
        Hair,
        Face,
        PartNum
    };

    struct UniformOutline
    {
        glm::vec<3, float, glm::highp> color = {1.0f, 0.0f, 0.0f};
        float factor_of_color = 1.0f;
        float width = 0.02f;

        bool operator==(const UniformOutline &rhs) const
        {
            return color == rhs.color && factor_of_color == rhs.factor_of_color && width == rhs.width;
        }
    };

    static const int MATERIAL_REGION_COUNT = 8;
    struct UniformPropertiesStarRail
    {
        UniformOutline outline;
        glm::vec4 specular_colors[MATERIAL_REGION_COUNT];
        float specular_shininess[MATERIAL_REGION_COUNT] = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f};
        float specular_roughness[MATERIAL_REGION_COUNT] = {.04f, .04f, .04f, .04f, .04f, .04f, .04f, .04f};

        bool operator==(const UniformPropertiesStarRail &rhs) const
        {
            return outline == rhs.outline &&
                   std::equal(specular_colors, specular_colors + MATERIAL_REGION_COUNT, rhs.specular_colors) &&
                   std::equal(specular_shininess, specular_shininess + MATERIAL_REGION_COUNT, rhs.specular_shininess) &&
                   std::equal(specular_roughness, specular_roughness + MATERIAL_REGION_COUNT, rhs.specular_roughness);
        }
    };

    struct UniformStarRailDebug
    {
        glm::vec4 debug_control;
        bool show_material_region;

        bool operator==(const UniformStarRailDebug &rhs) const
        {
            return debug_control == rhs.debug_control && show_material_region == rhs.show_material_region;
        }
    };

    class StarRailMaterialInstance : public IMaterialInstance
    {
    public:
        Material material;
        std::vector<vk::SharedDescriptorSet> descriptor_sets;
        UniformStarRailDebug buffer_data_debug;
        UniformPropertiesStarRail buffer_data_props;
        HostArrayBuffer<UniformStarRailDebug, true> uniform_buffer_debug;
        HostArrayBuffer<UniformPropertiesStarRail, true> uniform_buffer_props;
        Texture diffuse;
        Texture light_map;
        Texture cool_ramp;
        Texture warm_ramp;

        StarRailMaterialInstance(MaterialInstance &&inst) : material(std::move(inst.material)),
                                                            descriptor_sets(std::move(inst.descriptor_sets)),
                                                            buffer_data_debug(),
                                                            buffer_data_props(),
                                                            uniform_buffer_debug(),
                                                            uniform_buffer_props(),
                                                            diffuse(),
                                                            light_map(),
                                                            cool_ramp(),
                                                            warm_ramp() {}
        void update_descriptor_set();
        void update_descriptor_set_data(uint32_t cur_frame) override;
        RenderMaterialData get_render_data(uint32_t cur_frame) override
        {
            return {
                material.render_pipeline->pipeline.get(),
                material.render_pipeline->pipeline_layout.get(),
                descriptor_sets[cur_frame].get()};
        }
    };

    class StarRailMaterialBuilder
    {
    public:
        MaterialBuilder builder;

        StarRailMaterialBuilder(RenderPipelineResources &render_pipeline_resources,
                                PipelineLayoutBuilder pipeline_layout_builder,
                                PipelineBuilder pipeline_builder,
                                vk::SharedDevice device,
                                vk::PhysicalDevice physical_device,
                                vk::CommandBuffer command_buffer,
                                vk::Queue transfer_queue);

        Material build();
    };

    class StarRailMaterialInstanceBuilder
    {
    public:
        vk::SharedDevice device;
        vk::PhysicalDevice physical_device;
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        uint32_t frame_count;
        Material material;
        std::unordered_map<std::string, Texture> *texture_cache;
        std::string filename_diffuse;
        std::string filename_light_map;
        std::string filename_cool_ramp;
        std::string filename_warm_ramp;

        StarRailMaterialInstance build();
        std::shared_ptr<StarRailMaterialInstance> build_shared();
    };

    class StarRailOutlineMaterialInstance : public IMaterialInstance
    {
    public:
        Material material;
        std::vector<vk::SharedDescriptorSet> descriptor_sets;
        UniformStarRailDebug buffer_data_debug;
        UniformPropertiesStarRail buffer_data_props;
        HostArrayBuffer<UniformStarRailDebug, true> uniform_buffer_debug;
        HostArrayBuffer<UniformPropertiesStarRail, true> uniform_buffer_props;
        Texture diffuse;

        StarRailOutlineMaterialInstance(MaterialInstance &&inst) : material(std::move(inst.material)),
                                                                   descriptor_sets(std::move(inst.descriptor_sets)),
                                                                   buffer_data_debug(),
                                                                   buffer_data_props(),
                                                                   uniform_buffer_debug(),
                                                                   uniform_buffer_props(),
                                                                   diffuse() {}
        void update_descriptor_set();
        void update_descriptor_set_data(uint32_t cur_frame) override;
        RenderMaterialData get_render_data(uint32_t cur_frame) override
        {
            return {
                material.render_pipeline->pipeline.get(),
                material.render_pipeline->pipeline_layout.get(),
                descriptor_sets[cur_frame].get()};
        }
    };

    class StarRailOutlineMaterialBuilder
    {
    public:
        MaterialBuilder builder;

        StarRailOutlineMaterialBuilder(RenderPipelineResources &render_pipeline_resources,
                                       PipelineLayoutBuilder pipeline_layout_builder,
                                       PipelineBuilder pipeline_builder,
                                       vk::SharedDevice device,
                                       vk::PhysicalDevice physical_device,
                                       vk::CommandBuffer command_buffer,
                                       vk::Queue transfer_queue);

        Material build() { return builder.build(); }
    };

    class StarRailOutlineMaterialInstanceBuilder
    {
    public:
        vk::SharedDevice device;
        vk::PhysicalDevice physical_device;
        vk::CommandBuffer command_buffer;
        vk::Queue transfer_queue;
        uint32_t frame_count;
        Material material;
        std::unordered_map<std::string, Texture> *texture_cache;
        std::string filename_diffuse;

        StarRailOutlineMaterialInstance build();
        std::shared_ptr<StarRailOutlineMaterialInstance> build_shared()
        {
            return std::make_shared<StarRailOutlineMaterialInstance>(std::move(build()));
        }
    };
}