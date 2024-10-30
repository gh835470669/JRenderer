#pragma once

#include "jrenderer/window.h"
#include "jrenderer/instance.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/surface.h"
#include "jrenderer/swap_chain.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/image.h"
#include "jrenderer/frame.h"
#include "jrenderer/render_pass.h"
#include "jrenderer/command_pool.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/mesh.h"
#include "jrenderer/texture.h"
#include "jrenderer/descriptor.h"
#include "jrenderer/graphics_pipeline.h"
#include "jrenderer/uniform_buffer.h"
#include "jrenderer/shader.h"
#include "jrenderer/render_set.h"

#include <variant>

namespace jre
{
    struct GraphicsSetting
    {
        std::string app_name = "";
        uint32_t app_version = 1;
        std::string engine_name = "";
        uint32_t engine_version = 1;
        std::variant<bool, vk::PresentModeKHR> vsync = false; // true : FIFO  false : Mailbox
    };

    class Graphics
    {
    private:
        GraphicsSetting m_setting;

        gsl::not_null<const Window *> m_window;
        std::unique_ptr<Instance> m_instance;
        std::unique_ptr<PhysicalDevice> m_physical_device;
        std::unique_ptr<LogicalDevice> m_logical_device;
        std::unique_ptr<Surface> m_surface;
        std::unique_ptr<SwapChain> m_swap_chain;

        DepthImage2D m_depth_image;
        RenderPass m_render_pass;

        std::unique_ptr<const CommandPool> m_command_pool;
        std::unique_ptr<DescriptorPool> m_descriptor_pool;

        std::vector<std::unique_ptr<Frame>> m_frames;
        std::vector<Frame>::iterator m_current_frame;
        std::vector<std::unique_ptr<FrameBuffer>> m_frame_buffers;
        uint32_t m_current_frame_buffer_index = 0;

        void recreate_swapchain();

    public:
        Graphics(gsl::not_null<const Window *> window, const GraphicsSetting &setting = {});
        ~Graphics();

        inline const Instance *instance() const noexcept { return m_instance.get(); }
        const LogicalDevice *logical_device() const noexcept { return m_logical_device.get(); }
        const PhysicalDevice *physical_device() const noexcept { return m_physical_device.get(); }
        const RenderPass *render_pass() const noexcept { return &m_render_pass; }
        const SwapChain *swap_chain() const noexcept { return m_swap_chain.get(); }
        const DepthImage2D *depth_image() const noexcept { return &m_depth_image; }
        const CommandPool *command_pool() const noexcept { return m_command_pool.get(); }
        const DescriptorPool *descriptor_pool() const noexcept { return m_descriptor_pool.get(); }
        const GraphicsSetting &setting() const noexcept { return m_setting; }
        std::unique_ptr<DescriptorSet> create_descriptor_set(const std::vector<vk::DescriptorSetLayoutBinding> &bindings);

        static inline void check(const vk::Result &result) { vk::detail::resultCheck(result, "Graphics::check"); }

        void wait_idle() const;

        void draw(std::vector<IRenderSetRenderer *> renderers);

        vk::PresentModeKHR present_mode();
        inline const std::variant<bool, vk::PresentModeKHR> &vsync() const noexcept { return m_setting.vsync; }
        void set_vsync(const std::variant<bool, vk::PresentModeKHR> &vsync);
    };
}