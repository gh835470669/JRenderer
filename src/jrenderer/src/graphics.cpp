#include "jrenderer/graphics.h"
#include "jrenderer/render_set.h"

namespace jre
{

    Graphics::Graphics(gsl::not_null<const Window *> window, const GraphicsSetting &setting) : m_setting(setting),
                                                                                               m_window(window),
                                                                                               m_instance(std::make_unique<Instance>(InstanceCreateInfo{m_setting.app_name,
                                                                                                                                                        m_setting.app_version,
                                                                                                                                                        m_setting.engine_name,
                                                                                                                                                        m_setting.engine_version})),
                                                                                               m_physical_device(m_instance->create_first_physical_device()),
                                                                                               m_logical_device(m_physical_device->create_logical_device()),
                                                                                               m_surface(m_instance->create_surface(m_physical_device.get(), window->hinstance(), window->hwnd())),
                                                                                               m_swap_chain(std::move(m_logical_device->create_swapchain(*m_surface, nullptr,
                                                                                                                                                         SwapChainCreateInfo{window->size().x, window->size().y, setting.vsync}))),
                                                                                               m_depth_image(m_logical_device.get(), *m_physical_device, window->size().x, window->size().y),
                                                                                               m_render_pass(m_logical_device.get(), m_swap_chain->image_format(), m_depth_image.format()),
                                                                                               m_command_pool(std::make_unique<CommandPool>(m_logical_device.get(), CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_logical_device->graphics_queue_family()))),
                                                                                               m_descriptor_pool(std::make_unique<DescriptorPool>(m_logical_device.get(), DescriptorPoolCreateInfo{100, 100}))
    {
        for (uint32_t i = 0; i < m_swap_chain->images().size(); ++i)
        {
            std::vector<vk::ImageView> attachments{m_swap_chain->image_views()[i], m_depth_image.image_view()};
            m_frames.push_back(std::move(std::make_unique<Frame>(m_logical_device.get())));
            m_frame_buffers.push_back(std::move(std::make_unique<FrameBuffer>(m_logical_device.get(), m_render_pass, m_swap_chain->extent(), attachments, std::move(m_command_pool->allocate_command_buffer()))));
        }
    };

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
            m_render_pass.begin(command_buffer, *m_frame_buffers[m_current_frame_buffer_index], {{0, 0}, m_swap_chain->extent()}, vk::ClearColorValue(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f), vk::ClearDepthStencilValue(1.0f, 0));
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

    vk::PresentModeKHR Graphics::present_mode()
    {
        if (const bool *vsync = std::get_if<bool>(&m_setting.vsync))
        {
            return *vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
        }
        else
        {
            return std::get<vk::PresentModeKHR>(m_setting.vsync);
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

    std::unique_ptr<DescriptorSet> Graphics::create_descriptor_set(const std::vector<vk::DescriptorSetLayoutBinding> &bindings)
    {
        return std::make_unique<DescriptorSet>(m_logical_device.get(), m_descriptor_pool.get(),
                                               bindings);
    }
}
