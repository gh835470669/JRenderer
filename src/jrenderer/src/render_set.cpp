#include "jrenderer/render_set.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/graphics_pipeline.h"
#include "jrenderer/graphics.h"
#include "jrenderer/mesh.h"
#include "jrenderer/descriptor.h"

namespace jre
{
    vk::Viewport viewport::get_full_viewport(const vk::Extent2D &render_target_extent)
    {
        return vk::Viewport{0.0f, 0.0f, static_cast<float>(render_target_extent.width), static_cast<float>(render_target_extent.height), 0.0f, 1.0f};
    }

    vk::Rect2D scissor::get_full_scissor(const vk::Extent2D &render_target_extent)
    {
        return vk::Rect2D{vk::Offset2D(0, 0), render_target_extent};
    }

    void DefaultRenderSetRenderer::draw(const Graphics &graphics, const CommandBuffer &command_buffer)
    {
        if (graphics.swap_chain()->extent().width == 0 || graphics.swap_chain()->extent().height == 0)
            return;
        command_buffer.command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *render_set->graphics_pipeline);

        command_buffer.command_buffer().setViewport(0, func_get_viewport(graphics.swap_chain()->extent()));

        command_buffer.command_buffer().setScissor(0, func_get_scissor(graphics.swap_chain()->extent()));

        for (const auto &render_obj : render_set->render_objects)
        {
            update_uniform_buffer(*graphics.swap_chain(), render_obj);

            vk::DeviceSize offsets[] = {0};
            command_buffer.command_buffer().bindVertexBuffers(0, 1, &render_obj.get().get_mesh()->vertex_buffer().buffer(), offsets);
            command_buffer.command_buffer().bindIndexBuffer(render_obj.get().get_mesh()->index_buffer().buffer(), 0, vk::IndexType::eUint32);
            command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_set->graphics_pipeline->pipeline_layout(), 0, render_set->descriptor_set->descriptor_set(), nullptr);

            // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
            // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
            // firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
            // firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
            // command_buffer.draw(3, 1, 0, 0);
            command_buffer.command_buffer().drawIndexed(render_obj.get().get_mesh()->index_buffer().count(), 1, 0, 0, 0);
        }
    }

    void DefaultRenderSetRenderer::update_uniform_buffer(const SwapChain &swap_chian, const IRenderSetObject<> &render_obj) const
    {
        UniformBufferObject ubo{};
        ubo.model = render_obj.get_model_matrix();
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swap_chian.extent().width / (float)swap_chian.extent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        render_set->uniform_buffer->update_buffer(ubo);
    }
}
