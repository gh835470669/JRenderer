#pragma once

#include "jrenderer/descriptor_transform.h"
#include "jrenderer/drawer/command_buffer_recordable.h"
#include "jrenderer/pipeline.h"
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/material.h"
#include "jrenderer/camera/camera.h"
#include "jrenderer/camera/render_viewport.h"
#include "jrenderer/drawer/render_pass_drawer.h"
#include "jrenderer/ticker/scene_ticker.h"

namespace jre
{
    class IMesh;

    class Model
    {
    public:
        ModelTransform transform;
        std::shared_ptr<IMesh> mesh;
        std::vector<std::shared_ptr<IMaterialInstance>> materials;
    };

    class ModelFactory
    {
    public:
        ModelTransformFactory transform_factory;
        ModelFactory(vk::SharedDevice device, vk::PhysicalDevice physical_device, uint32_t frame_count = 1, uint32_t max_sets = 100) : transform_factory(device, physical_device, frame_count, max_sets) {}
        Model create() { return {transform_factory.create_transform(), nullptr, {}}; }
    };

    class Scene
    {
    public:
        DirectionalLight main_light;
        std::vector<Model> models;
        std::vector<RenderViewport> render_viewports;
        HostArrayBuffer<UniformScene> scene_buffers;
        vk::SharedDescriptorSetLayout descriptor_set_layout;
        std::vector<vk::SharedDescriptorSet> descriptor_sets;

        Scene(vk::SharedDevice device, vk::PhysicalDevice physical_device, uint32_t frame_count = 1)
            : models(), render_viewports(), descriptor_sets(frame_count)
        {
            scene_buffers = HostArrayBufferBuilder<UniformScene>(device, physical_device, std::array<UniformScene, 2>{})
                                .set_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                .build();
            auto [descriptor_pool, descriptor_set_layout_] = vk::shared::make_descriptor_pool_with_layout(
                device,
                frame_count,
                {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}}});
            descriptor_set_layout = descriptor_set_layout_;
            for (uint32_t i = 0; i < frame_count; ++i)
            {
                descriptor_sets[i] = vk::shared::allocate_one_descriptor_set(
                    descriptor_pool,
                    descriptor_set_layout.get());
                DescripterSetUpdater(descriptor_sets[i])
                    .write_uniform_buffer(vk::DescriptorBufferInfo{scene_buffers.vk_buffer(), sizeof(UniformScene) * i, sizeof(UniformScene)})
                    .update();
            }
        }
    };

    class DiffSceneMaterialBinder
    {
    public:
        DiffTrigger<std::tuple<vk::PipelineLayout, vk::DescriptorSet>> scene_descriptor_set_diff{};
        DiffTrigger<std::tuple<vk::PipelineLayout, vk::DescriptorSet>> model_descriptor_set_diff{};
        DiffTrigger<std::tuple<vk::PipelineLayout, vk::DescriptorSet>> material_descriptor_set_diff{};
        DiffTrigger<vk::Pipeline> pipeline_diff{};

        void bind(const RenderMaterialData &render_material_data,
                  vk::DescriptorSet scene_descriptor_set,
                  vk::DescriptorSet model_descriptor_set,
                  vk::CommandBuffer command_buffer);
    };

    class SceneDrawer : public RenderSubpassDrawable
    {
    public:
        ModelFactory factory;
        Scene scene;
        RenderPipelineResources render_pipelines;
        PipelineLayoutBuilder pipeline_layout_builder;
        PipelineBuilder pipeline_builder;
        SceneUBOTicker scene_ubo_ticker;
        SceneDrawer(Graphics &graphics);
        void on_draw(Graphics &graphics, vk::CommandBuffer command_buffer) override;
        void on_set_msaa(Graphics &graphics) override;
    };
}