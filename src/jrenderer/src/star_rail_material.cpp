#include "jrenderer/asset/star_rail_material.h"
#include "jrenderer/asset/raii_stb_image.h"
#include "jrenderer/descriptor_update.hpp"

namespace jre
{
    StarRailMaterialBuilder::StarRailMaterialBuilder(RenderPipelineResources &render_pipeline_resources,
                                                     PipelineLayoutBuilder pipeline_layout_builder,
                                                     PipelineBuilder pipeline_builder,
                                                     vk::SharedDevice device,
                                                     vk::PhysicalDevice physical_device,
                                                     vk::CommandBuffer command_buffer,
                                                     vk::Queue transfer_queue) : builder(render_pipeline_resources,
                                                                                         pipeline_layout_builder,
                                                                                         pipeline_builder,
                                                                                         device,
                                                                                         physical_device,
                                                                                         command_buffer,
                                                                                         transfer_queue)
    {
        builder.bindings = {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
                             {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
                             {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                             {3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                             {4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                             {5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};
        builder.vertex_shader_info.path = "res/shaders/star_rail_vert.spv";
        builder.fragment_shader_info.path = "res/shaders/star_rail_frag.spv";
    }

    Material StarRailMaterialBuilder::build()
    {
        return builder.build();
    }

    StarRailMaterialInstance StarRailMaterialInstanceBuilder::build()
    {
        StarRailMaterialInstance instance(material.create_instance(frame_count));
        instance.uniform_buffer_debug = UniformBufferBuilder<UniformStarRailDebug>(
                                            device, physical_device, frame_count)
                                            .build();
        instance.uniform_buffer_props = UniformBufferBuilder<UniformPropertiesStarRail>(
                                            device, physical_device, frame_count)
                                            .build();

        auto build_texture = [this](const std::string &filename, const vk::SamplerCreateInfo &sampler_create_info)
        {
            if (texture_cache && texture_cache->contains(filename))
            {
                return texture_cache->at(filename);
            }
            STBImage stb_data(filename);
            TextureData texture_data{stb_data.width(), stb_data.height(), stb_data.channels(), (unsigned char *)(stb_data.data())};
            TextureBuilder texture_builder(device, physical_device, command_buffer, transfer_queue, texture_data);
            texture_builder.set_sampler(sampler_create_info);
            return texture_cache ? texture_cache->emplace(filename, texture_builder.build()).first->second : texture_builder.build();
        };
        vk::SamplerCreateInfo sampler_create_info = make_sampler_create_info(vk::SamplerAddressMode::eRepeat);
        vk::SamplerCreateInfo ramp_sampler_create_info = make_sampler_create_info(vk::SamplerAddressMode::eClampToEdge);
        instance.diffuse = build_texture(filename_diffuse, sampler_create_info);
        instance.light_map = build_texture(filename_light_map, sampler_create_info);
        instance.cool_ramp = build_texture(filename_cool_ramp, ramp_sampler_create_info);
        instance.warm_ramp = build_texture(filename_warm_ramp, ramp_sampler_create_info);
        instance.update_descriptor_set();
        return instance;
    }

    std::shared_ptr<StarRailMaterialInstance> StarRailMaterialInstanceBuilder::build_shared()
    {
        return std::make_shared<StarRailMaterialInstance>(std::move(build()));
    }

    void StarRailMaterialInstance::update_descriptor_set_data(uint32_t cur_frame)
    {
        uniform_buffer_debug[cur_frame] = buffer_data_debug;
        uniform_buffer_props[cur_frame] = buffer_data_props;
    }

    void StarRailMaterialInstance::update_descriptor_set()
    {
        for (auto const [index, descriptor_set] : std::views::enumerate(descriptor_sets))
        {
            DescripterSetUpdater updater(descriptor_set);
            updater
                .write_uniform_buffer(uniform_buffer_debug)
                .write_uniform_buffer(uniform_buffer_props)
                .write_combined_image_sampler(diffuse.sampler.get(), diffuse.image_view.get())
                .write_combined_image_sampler(light_map.sampler.get(), light_map.image_view.get())
                .write_combined_image_sampler(cool_ramp.sampler.get(), cool_ramp.image_view.get())
                .write_combined_image_sampler(warm_ramp.sampler.get(), warm_ramp.image_view.get())
                .update();
        }
    }

    StarRailOutlineMaterialBuilder::StarRailOutlineMaterialBuilder(RenderPipelineResources &render_pipeline_resources,
                                                                   PipelineLayoutBuilder pipeline_layout_builder,
                                                                   PipelineBuilder pipeline_builder,
                                                                   vk::SharedDevice device,
                                                                   vk::PhysicalDevice physical_device,
                                                                   vk::CommandBuffer command_buffer,
                                                                   vk::Queue transfer_queue) : builder(render_pipeline_resources,
                                                                                                       pipeline_layout_builder,
                                                                                                       pipeline_builder,
                                                                                                       device,
                                                                                                       physical_device,
                                                                                                       command_buffer,
                                                                                                       transfer_queue)
    {
        builder.bindings = {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
                             {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
                             {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};
        builder.vertex_shader_info.path = "res/shaders/backface_outline_vert.spv";
        builder.fragment_shader_info.path = "res/shaders/backface_outline_frag.spv";
        builder.pipeline_builder.set_rasterizer(
            vk::PipelineRasterizationStateCreateInfo{{}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise, true, 10000.0f, 0.0f, 0.0f, 1.0f});
    }

    void StarRailOutlineMaterialInstance::update_descriptor_set_data(uint32_t cur_frame)
    {
        uniform_buffer_debug[cur_frame] = buffer_data_debug;
        uniform_buffer_props[cur_frame] = buffer_data_props;
    }

    void StarRailOutlineMaterialInstance::update_descriptor_set()
    {
        for (auto const [index, descriptor_set] : std::views::enumerate(descriptor_sets))
        {
            DescripterSetUpdater updater(descriptor_set);
            updater
                .write_uniform_buffer(uniform_buffer_debug)
                .write_uniform_buffer(uniform_buffer_props)
                .write_combined_image_sampler(diffuse.sampler.get(), diffuse.image_view.get())
                .update();
        }
    }

    StarRailOutlineMaterialInstance StarRailOutlineMaterialInstanceBuilder::build()
    {
        StarRailOutlineMaterialInstance instance(material.create_instance(frame_count));
        instance.uniform_buffer_debug = UniformBufferBuilder<UniformStarRailDebug>(
                                            device, physical_device, frame_count)
                                            .build();
        instance.uniform_buffer_props = UniformBufferBuilder<UniformPropertiesStarRail>(
                                            device, physical_device, frame_count)
                                            .build();

        vk::SamplerCreateInfo sampler_create_info = make_sampler_create_info(vk::SamplerAddressMode::eRepeat);
        auto build_texture = [this](const std::string &filename, const vk::SamplerCreateInfo &sampler_create_info)
        {
            if (texture_cache && texture_cache->contains(filename))
            {
                return texture_cache->at(filename);
            }
            STBImage stb_data(filename);
            TextureData texture_data{stb_data.width(), stb_data.height(), stb_data.channels(), (unsigned char *)(stb_data.data())};
            TextureBuilder texture_builder(device, physical_device, command_buffer, transfer_queue, texture_data);
            texture_builder.set_sampler(sampler_create_info);
            return texture_cache ? texture_cache->emplace(filename, texture_builder.build()).first->second : texture_builder.build();
        };

        instance.diffuse = build_texture(filename_diffuse, sampler_create_info);
        instance.update_descriptor_set();
        return instance;
    }
}