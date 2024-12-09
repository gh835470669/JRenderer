#include "jrenderer/star_rail_char_render_set.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/graphics_pipeline.h"
#include "jrenderer/graphics.h"
#include "jrenderer/mesh.h"
#include "jrenderer/descriptor.h"
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/utils/diff_trigger.hpp"

namespace jre
{
    void StarRailCharRenderSetRenderer::draw(const Graphics &graphics, const CommandBuffer &command_buffer)
    {
        std::function<void(ModelPart, ModelPart)> bind_pipeline = [&](ModelPart, ModelPart model_part)
        {
            switch (model_part)
            {
            case ModelPart::Body:
                command_buffer.command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *render_set->graphics_pipeline_body);
                command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_set->graphics_pipeline_body->pipeline_layout(), static_cast<int>(UniformBufferSetIndex::ePerRenderSet), render_set->descriptor_set->descriptor_set(), nullptr);
                break;
            case ModelPart::Face:
                command_buffer.command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *render_set->graphics_pipeline_face);
                command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_set->graphics_pipeline_face->pipeline_layout(), static_cast<int>(UniformBufferSetIndex::ePerRenderSet), render_set->descriptor_set->descriptor_set(), nullptr);
                break;
            case ModelPart::Hair:
                command_buffer.command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *render_set->graphics_pipeline_hair);
                command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_set->graphics_pipeline_hair->pipeline_layout(), static_cast<int>(UniformBufferSetIndex::ePerRenderSet), render_set->descriptor_set->descriptor_set(), nullptr);
                break;
            default:
                throw std::runtime_error("Unknown model part");
            }
        };
        DiffTrigger<ModelPart> bind_pipeline_trigger(ModelPart::PartNum, bind_pipeline);

        command_buffer.command_buffer().setViewport(0, func_get_viewport(graphics.swap_chain()->extent()));
        command_buffer.command_buffer().setScissor(0, func_get_scissor(graphics.swap_chain()->extent()));

        update_ubo_per_render_set();
        for (const auto &render_obj : render_set->render_objects)
        {
            update_ubo_per_obj(*graphics.swap_chain(), render_obj); // 这里并不会记录command buffer，其实相当于先把所有ubo更新了，然后再执行所有的draw call

            vk::DeviceSize offsets[] = {0};
            command_buffer.command_buffer().bindVertexBuffers(0, 1, &render_obj.get().mesh->vertex_buffer().buffer(), offsets);

            //     // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
            //     // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
            //     // firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
            //     // firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
            //     // command_buffer.draw(3, 1, 0, 0);
            for (size_t i = 0; i < render_obj.get().mesh->sub_meshes().size(); ++i)
            {
                const PmxSubMesh &sub_mesh = render_obj.get().mesh->sub_meshes()[i];
                const PmxMaterial &material = render_obj.get().sub_mesh_materials[i];
                command_buffer.command_buffer().bindIndexBuffer(render_obj.get().mesh->index_buffer().buffer(), sub_mesh.index_buffer().offset(), render_obj.get().mesh->index_buffer().vk_index_type());
                command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_set->get_graphics_pipeline(render_obj.get().model_parts[i])->pipeline_layout(), static_cast<int>(UniformBufferSetIndex::ePerObject), material.descriptor_set->descriptor_set(), nullptr);
                bind_pipeline_trigger.set_value(render_obj.get().model_parts[i]); // 如果值不一样，则触发绑定pipeline
                command_buffer.command_buffer().drawIndexed(sub_mesh.index_buffer().count(), 1, 0, 0, 0);
            }
        }
    }

    void StarRailCharRenderSetRenderer::update_ubo_per_obj(const SwapChain &swap_chian, PmxModel &render_obj) const
    {
        PmxUniformPerObject ubo_per_obj{};
        UniformMVP &ubo = ubo_per_obj.mvp;
        ubo.model = render_obj.get_model_matrix();
        ubo.view = render_set->view_matrix;
        ubo.proj = glm::perspective(glm::radians(45.0f), swap_chian.extent().width / (float)swap_chian.extent().height, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;
        for (auto &sub_mesh : render_obj.sub_mesh_materials)
            sub_mesh.uniform_buffer->update(ubo_per_obj);
    }

    void StarRailCharRenderSetRenderer::update_ubo_per_render_set()
    {
        PmxUniformPerRenderSet ubo_per_render_set(render_set->ubo.value());
        ubo_per_render_set.main_light = convert_to<UniformLight>(render_set->main_light);
        render_set->ubo.update(ubo_per_render_set);
    }

    vk::VertexInputBindingDescription PmxVertex::get_binding_description(uint32_t binding)
    {
        vk::VertexInputBindingDescription binding_description{};
        binding_description.binding = binding;
        binding_description.stride = sizeof(PmxVertex);
        binding_description.inputRate = vk::VertexInputRate::eVertex;
        return binding_description;
    }
    std::vector<vk::VertexInputAttributeDescription> PmxVertex::get_attribute_descriptions(uint32_t binding)
    {
        std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(3);
        attribute_descriptions[0].binding = binding;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attribute_descriptions[0].offset = offsetof(PmxVertex, position);

        attribute_descriptions[1].binding = binding;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attribute_descriptions[1].offset = offsetof(PmxVertex, normal);

        attribute_descriptions[2].binding = binding;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = vk::Format::eR32G32Sfloat;
        attribute_descriptions[2].offset = offsetof(PmxVertex, uv);
        return attribute_descriptions;
    }
    PipelineVertexInputState PmxVertex::get_pipeline_vertex_input_state(uint32_t binding)
    {
        return PipelineVertexInputState({PmxVertex::get_binding_description(binding)}, PmxVertex::get_attribute_descriptions(binding));
    }

    PmxSubMesh::PmxSubMesh(const PmxMesh &mesh, uint32_t offset, uint32_t count)
        : m_index_buffer(mesh.index_buffer(), offset * sizeof(uint32_t), count) {}

    PmxMesh::PmxMesh(gsl::not_null<const LogicalDevice *> device, const CommandBuffer &command_buffer, const PmxMeshData &mesh_data)
        : m_vertex_buffer(device, command_buffer, mesh_data.vertices), m_index_buffer(device, command_buffer, mesh_data.indices)
    {

        for (PmxMeshData::SubMeshRegion region : mesh_data.sub_mesh_indices)
        {
            m_sub_meshes.emplace_back(*this, region.first, region.second);
        }
    }
}
