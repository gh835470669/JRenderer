#include "jrenderer/frame.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/command_buffer.h"

namespace jre
{
    Frame::Frame(gsl::not_null<const LogicalDevice *> logical_device) : m_device(logical_device)
    {

        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
        // Outline of a frame
        // 1. Wait for the previous frame to finish
        // 2. Acquire an image from the swap chain
        // 3. Record a command buffer which draws the scene onto that image
        // 4. Submit the recorded command buffer
        // 5. Present the swap chain image

        // Synchronization  https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Synchronization
        // 程序员需要显式地指定哪些步骤是需要同步的，需要按顺序地执行的，需要你执行完我才执行的
        // (explicit)显式地指定同步规则，是Vulkan的设计原则之一
        // synchronization primitives 同步原语，是我们拥有告诉GPU怎么同步的指令
        // 1. Semaphores https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Semaphores 信号量/锁
        //  signal 表示我完成了，等待这个的线程可以执行了， wait 表示我还没执行，等待这个信号量signal后才执行
        // Semaphore 是让GPU同步的，CPU不会阻塞。下面的Fences是CPU同步的，CPU会阻塞。
        // 2. Fences https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Fences
        //  if the host needs to know when the GPU has finished something, we use a fence.

        // 下面这些步骤是需要同步的
        // 1. Acquire an image from the swap chain
        // 2. Execute commands that draw onto the acquired image
        // 3. Present that image to the screen for presentation, returning it to the swapchain

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.flags = vk::SemaphoreCreateFlags();
        m_image_available_semaphore = m_device->device().createSemaphore(semaphore_create_info);
        m_render_finished_semaphore = m_device->device().createSemaphore(semaphore_create_info);
        vk::FenceCreateInfo fence_create_info;
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
        m_in_flight_fence = m_device->device().createFence(fence_create_info);
    }

    Frame::~Frame()
    {
        m_device->device().destroySemaphore(m_image_available_semaphore);
        m_device->device().destroySemaphore(m_render_finished_semaphore);
        m_device->device().destroyFence(m_in_flight_fence);
    }

    FrameBuffer::FrameBuffer(gsl::not_null<const LogicalDevice *> logical_device, vk::RenderPass render_pass, vk::Extent2D extent, std::vector<vk::ImageView> attachments, std::unique_ptr<CommandBuffer> command_buffer)
        : m_device(logical_device), m_command_buffer(std::move(command_buffer))
    {
        vk::FramebufferCreateInfo frame_buffer_info;
        frame_buffer_info.renderPass = render_pass;
        frame_buffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        frame_buffer_info.pAttachments = attachments.data();
        frame_buffer_info.width = extent.width;
        frame_buffer_info.height = extent.height;
        frame_buffer_info.layers = 1;

        m_frame_buffer = logical_device->device().createFramebuffer(frame_buffer_info);
    }

    FrameBuffer::~FrameBuffer()
    {
        m_device->device().destroyFramebuffer(m_frame_buffer);
    }
}
