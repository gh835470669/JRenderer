#include "jrenderer/drawer/scene_drawer.h"
#include "jrenderer/graphics.h"
#include "jrenderer/utils/diff_trigger.hpp"
#include "jrenderer/mesh_drawer.h"
#include "tracy/Tracy.hpp"

namespace jre
{

    SceneDrawer::SceneDrawer(Graphics &graphics)
        : factory(graphics.logical_device(),
                  graphics.physical_device(),
                  graphics.cpu_frames().size()),
          pipeline_layout_builder{graphics.logical_device(), {}, {}},
          pipeline_builder(graphics.logical_device(), VK_NULL_HANDLE, graphics.render_pass().get()),
          scene(graphics.logical_device(), graphics.physical_device(), graphics.cpu_frames().size())
    {

        pipeline_layout_builder
            .descriptor_set_layouts.push_back(
                scene.descriptor_set_layout.get());
        pipeline_layout_builder
            .descriptor_set_layouts.push_back(
                factory.transform_factory.descriptor_set_layout.get());
        pipeline_builder
            .add_vertex_input_binding(get_binding_description<Vertex>(0))
            .add_vertex_input_attributes(get_attribute_descriptions<Vertex>(0))
            .add_dynamic_state(vk::DynamicState::eViewport)
            .add_dynamic_state(vk::DynamicState::eScissor)
            .enable_depth(true)
            .add_color_blend_attachment(PipelineBuilder::ColorBlendAttachment::alpha())
            .set_multisampling(graphics.settings().msaa)
            .set_rasterizer(vk::PolygonMode::eFill, 1.0f, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise);

        jre::RenderViewport &viewport = scene.render_viewports.emplace_back();
        viewport.viewport = vk::Viewport{0.0f, 0.0f, static_cast<float>(graphics.swapchain_extent().width), static_cast<float>(graphics.swapchain_extent().height), 0.0f, 1.0f};
    }

    void DiffSceneMaterialBinder::bind(const RenderMaterialData &render_material_data,
                                       vk::DescriptorSet scene_descriptor_set,
                                       vk::DescriptorSet model_descriptor_set,
                                       vk::CommandBuffer command_buffer)
    {
        if (pipeline_diff.update(render_material_data.pipeline))
        {
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, render_material_data.pipeline);
        }
        if (scene_descriptor_set_diff.update(std::make_tuple(render_material_data.pipeline_layout, scene_descriptor_set)))
        {
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_material_data.pipeline_layout, static_cast<int>(UniformBufferSetIndex::PerRenderSet), scene_descriptor_set, nullptr);
        }
        if (model_descriptor_set_diff.update(std::make_tuple(render_material_data.pipeline_layout, model_descriptor_set)))
        {
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_material_data.pipeline_layout, static_cast<int>(UniformBufferSetIndex::PerObject), model_descriptor_set, nullptr);
        }
        if (material_descriptor_set_diff.update(std::make_tuple(render_material_data.pipeline_layout, render_material_data.descriptor_set)))
        {
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_material_data.pipeline_layout, static_cast<int>(UniformBufferSetIndex::PerMaterial), render_material_data.descriptor_set, nullptr);
        }
    }

    void SceneDrawer::on_draw(Graphics &graphics, vk::CommandBuffer command_buffer)
    {
        ZoneScoped;
        DiffMeshBinder mesh_binder;
        DiffSceneMaterialBinder material_binder;

        for (auto &render_viewport : scene.render_viewports)
        {
            command_buffer.setViewport(0, render_viewport.viewport);
            command_buffer.setScissor(0, render_viewport.scissor.value_or(
                                             vk::Rect2D{{static_cast<int32_t>(render_viewport.viewport.x),
                                                         static_cast<int32_t>(render_viewport.viewport.y)},
                                                        {static_cast<uint32_t>(render_viewport.viewport.width),
                                                         static_cast<uint32_t>(render_viewport.viewport.height)}}));

            for (Model &model : scene.models)
            {
                const RenderMeshData mesh_data = model.mesh->get_render_data();
                mesh_binder.bind(mesh_data, command_buffer);

                auto cur_sub_mesh = mesh_data.sub_meshes.begin();
                auto cyclic_next = [](auto &it, auto &container) mutable
                { return std::next(it) == container.end() ? it = container.begin() : ++it; };

                for (auto &material : model.materials)
                {
                    RenderMaterialData render_material_data = material->get_render_data(graphics.current_cpu_frame());
                    material_binder.bind(render_material_data, scene.descriptor_sets[graphics.current_cpu_frame()].get(), model.transform.descriptor_sets[graphics.current_cpu_frame()].get(), command_buffer);

                    command_buffer.drawIndexed(cur_sub_mesh->index_count, 1, cur_sub_mesh->index_offset, cur_sub_mesh->vertex_offset, 0);
                    cyclic_next(cur_sub_mesh, mesh_data.sub_meshes);
                }
            }
        }
    }

    void SceneDrawer::on_set_msaa(Graphics &graphics)
    {
        for (auto &[key, render_pipeline] : render_pipelines.pipelines)
        {
            render_pipeline->pipeline_builder.render_pass = graphics.render_pass().get();
            render_pipeline->pipeline_builder.set_multisampling(graphics.settings().msaa);
            render_pipeline->recreate_pipeline();
        }
    }
}
