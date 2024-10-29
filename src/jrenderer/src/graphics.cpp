#include "jrenderer/graphics.h"
#include "jrenderer/render_set.h"

namespace jre
{
    void Graphics::draw(std::vector<IRenderSetRenderer *> renderers)
    {

        try
        {
            Frame &frame = *m_frames[0];
            check(m_logical_device->device().waitForFences({frame.in_flight_fence()}, true, std::numeric_limits<uint64_t>::max()));
            m_logical_device->device().resetFences({frame.in_flight_fence()});

            auto res = m_logical_device->device().acquireNextImageKHR(m_swap_chain->swapchain(), std::numeric_limits<uint64_t>::max(), frame.image_available_semaphore(), nullptr);
            m_current_frame_buffer_index = res.value;

            const CommandBuffer &command_buffer = *m_frame_buffers[m_current_frame_buffer_index]->command_buffer();
            command_buffer.command_buffer().reset();
            command_buffer.begin();
            m_render_pass.begin(command_buffer, *m_frame_buffers[m_current_frame_buffer_index], {{0, 0}, m_swap_chain->extent()}, vk::ClearColorValue(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f), vk::ClearDepthStencilValue(1.0f, 0.0f));
            for (auto &renderer : renderers)
            {
                renderer->draw(*this, command_buffer);
            }

            m_render_pass.end(command_buffer);
            command_buffer.end();
            command_buffer.submit(m_logical_device->graphics_queue(), frame.image_available_semaphore(), {vk::PipelineStageFlagBits::eColorAttachmentOutput}, frame.render_finished_semaphore(), frame.in_flight_fence());

            m_logical_device->present(*m_swap_chain, m_current_frame_buffer_index, {frame.render_finished_semaphore()});
        }
        catch (vk::OutOfDateKHRError)
        {
            recreate_swapchain();
            return;
        }
    }

    void Graphics::set_vsync(const std::variant<bool, vk::PresentModeKHR> &vsync)
    {
        if (m_setting.vsync == vsync)
            return;
        m_setting.vsync = vsync;
        recreate_swapchain();
    }

    void Graphics::recreate_swapchain()
    {
        wait_idle();

        m_swap_chain->recreate(
            *m_surface,
            SwapChainCreateInfo{m_window->width(), m_window->height(), m_setting.vsync});

        m_frames.clear();
        m_frame_buffers.clear();
        for (uint32_t i = 0; i < m_swap_chain->images().size(); ++i)
        {
            std::vector<vk::ImageView> attachments{m_swap_chain->image_views()[i], m_depth_image.image_view()};
            m_frames.push_back(std::move(std::make_unique<Frame>(m_logical_device.get())));
            m_frame_buffers.push_back(std::move(std::make_unique<FrameBuffer>(m_logical_device.get(), m_render_pass, m_swap_chain->extent(), attachments, std::move(m_command_pool->allocate_command_buffer()))));
        }
    }

    Graphics::~Graphics()
    {
        wait_idle();
    }

    void Graphics::wait_idle() const

    {
        m_logical_device->device().waitIdle();
    }

    std::shared_ptr<DescriptorSet> Graphics::create_descriptor_set(const std::vector<vk::DescriptorSetLayoutBinding> &bindings)
    {
        return std::make_shared<DescriptorSet>(m_logical_device.get(), m_descriptor_pool.get(),
                                               bindings);
    }
}
