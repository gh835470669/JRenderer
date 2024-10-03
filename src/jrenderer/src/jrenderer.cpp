#include "jrenderer.h"
#include <iostream>
#include "tracy/Tracy.hpp"

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_pipeline_builder(window.hinstance(), window.hwnd(),
                                                              VulkanPipelineSwapChainBuilder(
                                                                  std::get<0>(m_window.size()),
                                                                  std::get<1>(m_window.size()),
                                                                  m_render_setting.vsync)),
                                           m_pipeline(m_pipeline_builder.Build()),
                                           m_imgui_context(*m_pipeline, window)
    {
        m_window.message_handlers.push_back(m_imgui_context);

        VulkanBufferCreateInfo create_info;
        create_info.size = sizeof(vertices[0]) * vertices.size();
        VulkanMemoryCreateInfo memory_create_info;

        create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
        memory_create_info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        VulkanBufferHandle vertex_staging_buffer = m_pipeline->create_buffer_with_memory(create_info, memory_create_info);
        vertex_staging_buffer.map_memory(vertices);

        create_info.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
        memory_create_info.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        m_vertex_buffer = std::make_shared<VulkanBufferHandle>(m_pipeline->create_buffer_with_memory(create_info, memory_create_info));

        copy_buffer(vertex_staging_buffer, *m_vertex_buffer, create_info.size);
        vertex_staging_buffer.destroy_with_memory();

        create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
        create_info.size = sizeof(indices[0]) * indices.size();
        memory_create_info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        VulkanBufferHandle index_staging_buffer = m_pipeline->create_buffer_with_memory(create_info, memory_create_info);
        index_staging_buffer.map_memory(indices);

        create_info.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
        memory_create_info.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        m_index_buffer = std::make_shared<VulkanBufferHandle>(m_pipeline->create_buffer_with_memory(create_info, memory_create_info));

        copy_buffer(index_staging_buffer, *m_index_buffer, create_info.size);
        index_staging_buffer.destroy_with_memory();
    }

    JRenderer::~JRenderer()
    {
    }

    void JRenderer::new_imgui_frame()
    {
        m_imgui_context.new_frame();
    }

    void JRenderer::new_frame()
    {
        // m_imgui_context.draw();
        try
        {
            ZoneScoped;
            jre::VulkanPipeline::VulkanPipelineDrawContext pipeline_draw_context = m_pipeline->BeginDraw();
            jre::DrawContext draw_context =
                {
                    pipeline_draw_context.command_buffer(),
                };
            draw(draw_context);
            m_imgui_context.draw(draw_context);
        }
        catch (vk::OutOfDateKHRError)
        {
            // 当窗口的大小变化的时候，这个异常会爆出来
            // 一般是presentKHR爆出来的
            // C接口的话是判断presentKHR的返回值，https://github.com/KhronosGroup/Vulkan-Hpp/issues/274 这里会说，我自己创造性的用捕捉异常的方式解决这个问题，看来我C++理解的还是不错><
            // 处理参考代码https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Recreating-the-swap-chain
            // 这个重新创建需要在presentKHR之后，为的是保证m_render_finished_semaphore信号量被signal了
            // 如果需要显式的改变分辨率，就用触发变量 https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Handling-resizes-explicitly
            ReInitSwapChain();
            // present error
            // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a 【window resize】.
            // VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly. 这个还没试过，如果这个出现也是ReInitSwapChain
        }
    }
    void JRenderer::ReInitSwapChain()
    {
        auto size = m_window.size();
        m_pipeline_builder.swap_chain_builder.SetSize(std::get<0>(size), std::get<1>(size));
        m_pipeline->ReInitSwapChain(m_pipeline_builder.swap_chain_builder.Build(*m_pipeline));
    }

    void JRendererRebuilder::setVsync(bool vsync)
    {
        if (m_renderer.m_render_setting.vsync == vsync)
            return;
        m_renderer.m_render_setting.vsync = vsync;
        m_renderer.m_pipeline_builder.swap_chain_builder.SetVsync(vsync);
        m_renderer.m_pipeline->ReInitSwapChain(m_renderer.m_pipeline_builder.swap_chain_builder.Build(*m_renderer.m_pipeline));
    }

    void JRenderer::draw(const DrawContext &draw_context)
    {
        auto &command_buffer = draw_context.command_buffer;
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->graphics_pipeline());

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_pipeline->swap_chain_extent().width);
        viewport.height = static_cast<float>(m_pipeline->swap_chain_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        command_buffer.setViewport(0, viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = m_pipeline->swap_chain_extent();
        command_buffer.setScissor(0, scissor);

        vk::Buffer vertex_buffers[] = {static_cast<vk::Buffer>(*m_vertex_buffer)};
        vk::DeviceSize offsets[] = {0};
        command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);

        command_buffer.bindIndexBuffer(static_cast<vk::Buffer>(*m_index_buffer), 0, vk::IndexType::eUint16);

        // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
        // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
        // firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
        // firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        // command_buffer.draw(3, 1, 0, 0);
        command_buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    void JRenderer::copy_buffer(const VulkanBufferHandle &src_buffer, const VulkanBufferHandle &dst_buffer, size_t size)
    {
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = m_pipeline->command_pool();
        alloc_info.commandBufferCount = 1;

        vk::CommandBuffer command_buffer = m_pipeline->device().allocateCommandBuffers(alloc_info)[0];

        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        command_buffer.begin(begin_info);

        vk::BufferCopy copy_region{};
        copy_region.size = size;
        command_buffer.copyBuffer(src_buffer.buffer(), dst_buffer.buffer(), copy_region);

        command_buffer.end();

        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        m_pipeline->graphics_queue().submit(submit_info, nullptr);
        m_pipeline->graphics_queue().waitIdle();

        m_pipeline->device().freeCommandBuffers(m_pipeline->command_pool(), command_buffer);
    }

} // namespace jre
