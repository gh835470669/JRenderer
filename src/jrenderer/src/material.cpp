#include "jrenderer/material.h"
#include "jrenderer/pipeline.h"
#include <glm/gtx/hash.hpp>

namespace jre
{
    Material MaterialBuilder::build()
    {
        auto get_or_create_shader = [this](const std::string &path)
        {
            if (!shader_cache)
            {
                return vk::shared::create_shader_from_spv_file(device, path);
            }

            auto [it, inserted] = shader_cache->try_emplace(path);
            if (inserted)
            {
                it->second = vk::shared::create_shader_from_spv_file(device, path);
            }
            return it->second;
        };
        Material material;
        std::tie(material.descriptor_pool, material.descriptor_set_layout) = vk::shared::make_descriptor_pool_with_layout(
            device,
            descriptor_set_count,
            bindings);

        size_t hash_of_pipeline = render_pipeline_resources.hasher(
            PipelineHashInfo{
                vertex_shader_info.path,
                fragment_shader_info.path,
                vertex_shader_info.constants,
                fragment_shader_info.constants});
        auto [it, inserted] = render_pipeline_resources.pipelines.try_emplace(hash_of_pipeline,
                                                                              std::make_shared<RenderPipeline>(
                                                                                  RenderPipeline{
                                                                                      vk::SharedPipeline{},
                                                                                      vk::SharedPipelineLayout{},
                                                                                      vk::SharedShaderModule{},
                                                                                      vk::SharedShaderModule{},
                                                                                      pipeline_builder}));
        if (inserted)
        {
            it->second->vertex_shader = get_or_create_shader(vertex_shader_info.path);
            it->second->fragment_shader = get_or_create_shader(fragment_shader_info.path);
            pipeline_layout_builder.descriptor_set_layouts.push_back(
                material.descriptor_set_layout.get());
            it->second->pipeline_layout = pipeline_layout_builder.build();
            it->second->pipeline_builder.pipeline_layout = it->second->pipeline_layout.get();
            it->second->pipeline_builder
                .add_vertex_shader(it->second->vertex_shader.get(), vertex_shader_info.constants, vertex_shader_info.entry)
                .add_fragment_shader(it->second->fragment_shader.get(), fragment_shader_info.constants, fragment_shader_info.entry);
            it->second->recreate_pipeline();
        }
        material.render_pipeline = it->second;
        return material;
    }

    MaterialInstance Material::create_instance(uint32_t frame_count)
    {
        return MaterialInstance(*this, vk::shared::allocate_descriptor_sets(descriptor_pool,
                                                                            std::vector<vk::DescriptorSetLayout>(frame_count, descriptor_set_layout.get())));
    }
}