#pragma once

#include "jrenderer/tick_draw.h"
#include "jrenderer/window.h"
#include "jrenderer/resources.hpp"
#include "jrenderer/image.h"
#include "jrenderer/frame.h"
#include "jrenderer/render_pass.h"
#include "jrenderer/drawer/render_pass_drawer.h"
#include "jrenderer/command_buffer.h"
#include "jrenderer/mesh.h"
#include "jrenderer/texture.h"
#include "jrenderer/descriptor_transform.h"
#include "jrenderer/pipeline.h"
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <any>
#include <variant>

namespace jre
{

    struct GraphicsSettings
    {
        struct InstanceSettings
        {
            std::string app_name = "";
            uint32_t app_version = 1;
            std::string engine_name = "";
            uint32_t engine_version = 1;
        };

        InstanceSettings instance_settings;
        std::variant<bool, vk::PresentModeKHR> vsync = true;         // true : FIFO  false : Mailbox
        vk::SampleCountFlagBits msaa = vk::SampleCountFlagBits::e16; // 1 : No MSAA  2 : 2xMSAA  4 : 4xMSAA ... etc 超出GPU支持的值会被截断
    };

    struct PhysicalDeviceInfo
    {
        vk::PhysicalDeviceProperties properties;
        vk::PhysicalDeviceFeatures features;
        vk::PhysicalDeviceMemoryProperties memory_properties;

        vk::SampleCountFlagBits max_sample_count() const noexcept
        {
            vk::SampleCountFlags flags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
            return static_cast<vk::SampleCountFlagBits>(std::bit_floor(VkSampleCountFlags(flags)));
        }
    };

    using CPUFrameResources = std::vector<std::any>;
    using DrawFunc = std::function<void(Graphics &, vk::CommandBuffer)>;
    using ResizeFunc = std::function<void(uint32_t, uint32_t)>;

    class Graphics : public Drawable
    {
    public:
        Graphics(gsl::not_null<const Window *> window, const GraphicsSettings &settings = {});
        Graphics(const Graphics &) = delete; // non-copyable
        Graphics &operator=(const Graphics &) = delete;
        Graphics(Graphics &&) = delete; // non-movable
        Graphics &operator=(Graphics &&) = delete;
        ~Graphics();

        vk::raii::Context &context() noexcept { return m_context; }
        vk::SharedInstance &instance() noexcept { return m_instance; }
        vk::PhysicalDevice &physical_device() noexcept { return m_physical_device; }
        const PhysicalDeviceInfo &physical_device_info() noexcept { return m_physical_device_info; }
        vk::SharedDevice &logical_device() noexcept { return m_logical_device; }
        vk::SharedQueue &graphics_queue() noexcept { return m_graphics_queue; }
        vk::SharedQueue &present_queue() noexcept { return m_present_queue; }
        vk::SharedQueue &transfer_queue() noexcept { return m_transfer_queue; }
        vk::SharedSurfaceKHR surface() noexcept { return m_swapchain.getSurface(); }
        vk::SurfaceFormatKHR surface_format() noexcept { return m_surface_format; }
        vk::SharedSwapchainKHR &swapchain() noexcept { return m_swapchain; }
        vk::Extent2D swapchain_extent() noexcept { return m_swapchain_extent; }
        std::vector<vk::SharedImageView> &swapchain_image_views() noexcept { return m_swapchain_image_views; }
        DeviceImage &msaa_image_data() noexcept { return m_msaa_image_data; }
        vk::SharedRenderPass &render_pass() noexcept { return m_render_pass; }
        vk::SharedCommandPool &graphics_command_pool() noexcept { return m_graphics_command_pool; }
        vk::SharedCommandPool &transfer_command_pool() noexcept { return m_transfer_command_pool; }
        std::vector<vk::SharedFramebuffer> &framebuffers() noexcept { return m_framebuffers; }
        std::vector<CPUFrame> &cpu_frames() noexcept { return m_cpu_frames; }
        uint32_t current_cpu_frame() noexcept { return static_cast<uint32_t>(std::distance(m_cpu_frames.begin(), m_current_cpu_frame)); }
        ModelTransformFactory &model_transform_manager() noexcept { return m_model_transform_manager; }
        const GraphicsSettings &settings() const noexcept { return m_settings; }
        std::vector<std::vector<std::vector<std::shared_ptr<CommandBufferRecordable>>>> &render_pass_renderers() noexcept { return m_render_pass_renderers; }
        RenderPassDrawers &render_pass_drawer() noexcept { return m_render_pass_drawer; }

        static inline void check(const vk::Result &result) { vk::detail::resultCheck(result, "Graphics::check"); }

        void wait_idle() const;

        inline bool preset_visible(bool) override { return !is_minimized(); } // 这是最舒服的写法了，当最小化的时候会让swapchian长宽为0，其他地方会报错，否则就得到处判断。这个对性能的影响我看不大就这样吧。
        void on_draw() override;

        vk::PresentModeKHR present_mode();
        inline const std::variant<bool, vk::PresentModeKHR> &vsync() const noexcept { return m_settings.vsync; }
        void set_vsync(const std::variant<bool, vk::PresentModeKHR> &vsync);
        void set_msaa(const vk::SampleCountFlagBits &msaa);

        inline bool is_minimized() const;

    private:
        gsl::not_null<const Window *> m_window;

        vk::raii::Context m_context;
        vk::SharedInstance m_instance;
        vk::PhysicalDevice m_physical_device;
        PhysicalDeviceInfo m_physical_device_info;
        // 不需要保存shared surface了，shared swapchain 会引用住
        vk::SurfaceFormatKHR m_surface_format;

        vk::SharedDevice m_logical_device;
        uint32_t m_graphics_queue_family_index;
        uint32_t m_present_queue_family_index;
        uint32_t m_transfer_queue_family_index;
        vk::SharedQueue m_graphics_queue;
        vk::SharedQueue m_present_queue;
        vk::SharedQueue m_transfer_queue;

        vk::SharedSwapchainKHR m_swapchain;
        vk::Extent2D m_swapchain_extent;
        std::vector<vk::SharedImageView> m_swapchain_image_views;
        std::list<DeviceImage> m_depth_images;
        DeviceImage m_msaa_image_data;

        vk::SharedRenderPass m_render_pass;

        vk::SharedCommandPool m_graphics_command_pool;
        vk::SharedCommandPool m_transfer_command_pool;

        std::vector<vk::SharedFramebuffer> m_framebuffers;
        uint32_t m_current_frame_buffer_index = 0;
        std::vector<vk::ClearValue> m_clear_values;
        std::vector<CPUFrame> m_cpu_frames;
        std::vector<CPUFrame>::iterator m_current_cpu_frame;

        ModelTransformFactory m_model_transform_manager;

        RenderPassDrawers m_render_pass_drawer;
        std::vector<std::vector<std::vector<std::shared_ptr<CommandBufferRecordable>>>> m_render_pass_renderers;

        GraphicsSettings m_settings;

        void create_instance();
        void pick_physical_device();
        void correct_settings();
        void create_logical_device_and_queue();
        vk::SharedSurfaceKHR create_surface();
        void create_surface_and_swapchain();
        void recreate_swapchain();
        void create_framebuffers();
        void create_render_pass();
        void create_command_pool();
        void create_cpu_frames();

    public:
        std::list<ResizeFunc> resize_funcs;
    };
}