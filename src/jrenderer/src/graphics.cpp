#include "jrenderer/graphics.h"
#include <fmt/core.h>
#include <vulkan_utils/utils.hpp>
#include "jrenderer/utils/vk_shared_utils.h"
#include "jrenderer/utils/vk_utils.h"
#include "jrenderer/ticker/scene_ticker.h"
#include "jrenderer/drawer/scene_drawer.h"

namespace jre
{

    Graphics::Graphics(gsl::not_null<const Window *> window,
                       const GraphicsSettings &settings)
        : m_window(window), m_settings(settings)
    {
        create_instance();
        pick_physical_device();
        correct_settings();
        create_logical_device_and_queue();
        create_surface_and_swapchain();
        create_command_pool();
        create_render_pass();
        create_framebuffers();
        create_cpu_frames();
        m_model_transform_manager = ModelTransformFactory(m_logical_device, m_physical_device);
    };

    void Graphics::create_instance()

    {
        const GraphicsSettings::InstanceSettings &settings = m_settings.instance_settings;
        vk::ApplicationInfo application_info(settings.app_name.c_str(),
                                             settings.app_version,
                                             settings.engine_name.c_str(),
                                             settings.engine_version,
                                             VK_API_VERSION_1_3);
        std::vector<std::string> wanted_layers = {};
        // 1. check layer support
        // 2. add validation layer for debug
        std::vector<char const *> enabled_layers = vk::su::gatherLayers(wanted_layers
#if !defined(NDEBUG)
                                                                        ,
                                                                        vk::enumerateInstanceLayerProperties()
#endif
        );
        std::vector<std::string> wanted_extensions = {};
        // surface extensions
        wanted_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        wanted_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        wanted_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_VI_NN)
        wanted_extensions.push_back(VK_NN_VI_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        wanted_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        wanted_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        wanted_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        wanted_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
        wanted_extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
#endif

        // 1. check extension support
        // 2. add VK_EXT_DEBUG_UTILS_EXTENSION_NAME for debug
        std::vector<char const *> enabled_extensions = vk::su::gatherExtensions(wanted_extensions
#if !defined(NDEBUG)
                                                                                ,
                                                                                vk::enumerateInstanceExtensionProperties()
#endif
        );

        m_instance = vk::SharedInstance(
            vk::createInstance(
                vk::su::makeInstanceCreateInfoChain({}, application_info, enabled_layers, enabled_extensions).get<vk::InstanceCreateInfo>() // if debug, add vk::su::debugUtilsMessengerCallback
                ));
    }

    void Graphics::pick_physical_device()
    {
        m_physical_device = m_instance->enumeratePhysicalDevices().front();
        m_physical_device_info = PhysicalDeviceInfo{
            m_physical_device.getProperties(),
            m_physical_device.getFeatures(),
            m_physical_device.getMemoryProperties()};
    }

    void Graphics::correct_settings()
    {
        vk::SampleCountFlagBits max_msaa = m_physical_device_info.max_sample_count();
        auto wanted_msaa = std::exchange(m_settings.msaa, std::min(m_settings.msaa, max_msaa));
        fmt::print("wanted msaa: {}, cur msaa: {} (if not the same, GPU do not support)\n", static_cast<int>(wanted_msaa), static_cast<int>(m_settings.msaa));
    }

    vk::SharedSurfaceKHR Graphics::create_surface()
    {
        vk::Win32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.hinstance = m_window->hinstance();
        surface_create_info.hwnd = m_window->hwnd();
        surface_create_info.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
        vk::SurfaceKHR surface;
        vk::Result res = m_instance->createWin32SurfaceKHR(&surface_create_info, nullptr, &surface);
        vk::detail::resultCheck(res, "createWin32SurfaceKHR");

        // pick surface format
        for (auto &format : m_physical_device.getSurfaceFormatsKHR(surface))
        {
            if (format.format == vk::Format::eR8G8B8A8Unorm)
            {
                m_surface_format = format;
                break;
            }
        }
        assert(m_surface_format.format != vk::Format::eUndefined);

        return vk::SharedSurfaceKHR{surface, m_instance};
    }

    void Graphics::create_logical_device_and_queue()
    {
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkSharingMode.html
        // sharing mode 是与queue family是否一致有关，而不是与是否同一个queue有关
        // 1. 尽量让queue同属于同一个queue family，这样Buffer and image sharing modes 可以选择exclusive，访问性能会比concurrent好
        // 2. 记录下来queue family index，给buffer和image的sharing mode决策使用
        auto properties = m_physical_device.getQueueFamilyProperties();
        bool found_graphics = false;
        bool found_present = false;
        bool found_transfer = false;
        for (int i = 0; i < properties.size(); i++)
        {
            if (properties[i].queueFlags & vk::QueueFlagBits::eTransfer)
            {
                m_transfer_queue_family_index = i;
                found_transfer = true;
            }
            if (m_physical_device.getWin32PresentationSupportKHR(i) && properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                m_graphics_queue_family_index = i;
                m_present_queue_family_index = i;
                found_present = true;
                found_graphics = true;
            }

            if (found_graphics && found_present && found_transfer)
                break;
        }

        std::map<uint32_t, int> unique_queue_families = {{m_graphics_queue_family_index, 0}, {m_present_queue_family_index, 0}, {m_transfer_queue_family_index, 0}};
        unique_queue_families[m_graphics_queue_family_index]++;
        unique_queue_families[m_present_queue_family_index]++;
        unique_queue_families[m_transfer_queue_family_index]++;

        vk::DeviceCreateInfo create_info;
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        std::vector<std::vector<float>> priorities;
        for (auto [queue_family, queue_num] : unique_queue_families)
        {
            priorities.emplace_back(queue_num, 1.0f);
            queue_create_infos.push_back(vk::DeviceQueueCreateInfo()
                                             .setQueuePriorities(priorities.back())
                                             .setQueueCount(queue_num)
                                             .setQueueFamilyIndex(queue_family));
        }

        // Create logic device from physical device
        std::array<const char *, 1> wanted_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; // swap chain 需要定义extension，否则会段错误，它应该是dll来的

        vk::PhysicalDeviceVulkan12Features features;
        features.setScalarBlockLayout(true); // shader中使用的layout有std430，#extension GL_EXT_scalar_block_layout : enable。需要在这里设置，否则validation layer会报错

        create_info.setQueueCreateInfos(queue_create_infos)
            .setQueueCreateInfoCount(static_cast<uint32_t>(queue_create_infos.size()))
            .setEnabledExtensionCount(static_cast<uint32_t>(wanted_extensions.size()))
            .setPpEnabledExtensionNames(wanted_extensions.data())
            .setPNext(&features);
        m_logical_device = vk::SharedDevice{m_physical_device.createDevice(create_info)}; // 没有引用住SharedInstance，会报错。graphics以外的对象(windows的回调)引用住vulkan资源就会有问题

        m_graphics_queue = vk::SharedQueue{m_logical_device->getQueue(m_graphics_queue_family_index, --unique_queue_families[m_graphics_queue_family_index]), m_logical_device};
        m_present_queue = vk::SharedQueue{m_logical_device->getQueue(m_present_queue_family_index, --unique_queue_families[m_present_queue_family_index]), m_logical_device};
        m_transfer_queue = vk::SharedQueue{m_logical_device->getQueue(m_transfer_queue_family_index, --unique_queue_families[m_transfer_queue_family_index]), m_logical_device};
    }

    void Graphics::create_surface_and_swapchain()
    {
        vk::SharedSurfaceKHR shared_surface = m_swapchain ? m_swapchain.getSurface() : create_surface();

        // Swapchain 和具体的操作系统，窗口系统相关的
        const vk::SurfaceCapabilitiesKHR &surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(shared_surface.get());

        // 长宽
        vk::Extent2D swapchain_extent(m_window->size().x, m_window->size().y);
        if (surface_capabilities.currentExtent.width == (std::numeric_limits<uint32_t>::max)())
        {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swapchain_extent.width = std::clamp<uint32_t>(swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            swapchain_extent.height = std::clamp<uint32_t>(swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
        }
        else
        {
            // If the surface size is defined, the swap chain size must match
            swapchain_extent = surface_capabilities.currentExtent;
        }

        // 垂直同步
        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eMailbox;
        try
        {
            present_mode = std::get<bool>(m_settings.vsync) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
        }
        catch (const std::bad_variant_access &)
        {
            present_mode = std::get<vk::PresentModeKHR>(m_settings.vsync);
        }

        // 变换和透明度操作还没知道有什么应用
        vk::SurfaceTransformFlagBitsKHR pre_transform = (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
                                                            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                            : surface_capabilities.currentTransform;
        vk::CompositeAlphaFlagBitsKHR composite_alpha =
            (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
            : (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
            : (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)        ? vk::CompositeAlphaFlagBitsKHR::eInherit
                                                                                                              : vk::CompositeAlphaFlagBitsKHR::eOpaque;

        // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
        // Frame Buffer and Swapchain
        // a queue of images to present on the screen
        vk::SwapchainCreateInfoKHR swap_chain_create_info;
        swap_chain_create_info
            .setOldSwapchain(m_swapchain.get())
            .setPresentMode(present_mode)                            // vsync 相关的
            .setPreTransform(pre_transform)                          // 设置到屏幕前进行变换：不变
            .setCompositeAlpha(composite_alpha)                      // 不透明
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment) // 颜色附件 GPU才能写  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc ?
            .setImageArrayLayers(1)                                  // 一张图
            .setClipped(true)                                        // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
            .setSurface(shared_surface.get())
            .setImageFormat(m_surface_format.format)
            .setImageColorSpace(m_surface_format.colorSpace)
            .setImageExtent(swapchain_extent)
            // However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum:
            // 这个会让CPU等？得留意一下不同数量的图会对性能有什么影响
            // 我的电脑最少2张，最多8张耶
            .setMinImageCount(std::clamp<uint32_t>(1u,
                                                   surface_capabilities.minImageCount,
                                                   surface_capabilities.maxImageCount));

        if (m_graphics_queue_family_index != m_present_queue_family_index)
        {
            uint32_t queue_family_indices[2] = {m_graphics_queue_family_index, m_present_queue_family_index};
            // If the graphics and present queues are from different queue families, we either have to explicitly transfer
            // ownership of images between the queues, or we have to create the swapchain with imageSharingMode as
            // vk::SharingMode::eConcurrent
            swap_chain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            swap_chain_create_info.setQueueFamilyIndices(queue_family_indices);
        }
        else
        {
            swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
            swap_chain_create_info.setQueueFamilyIndices({m_present_queue_family_index});
        }

        m_swapchain = vk::SharedSwapchainKHR(m_logical_device->createSwapchainKHR(swap_chain_create_info), m_logical_device, shared_surface);
        m_swapchain_extent = swapchain_extent;

        // Get SwapChain Images and Create Image Views
        m_swapchain_image_views = m_logical_device->getSwapchainImagesKHR(m_swapchain.get()) |
                                  std::views::transform(
                                      [this](const vk::Image &image)
                                      {
                                        vk::ImageViewCreateInfo image_view_create_info;
                                        image_view_create_info
                                            .setImage(image)
                                            .setViewType(vk::ImageViewType::e2D)
                                            .setFormat(m_surface_format.format)
                                            // The components field allows you to swizzle the color channels around. For example, you can map all of the channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a channel. In our case we'll stick to the default mapping.
                                            .setComponents(vk::ComponentMapping())
                                            // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
                                            .setSubresourceRange(
                                                vk::ImageSubresourceRange()
                                                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                    .setBaseMipLevel(0)
                                                    .setLevelCount(1)
                                                    .setBaseArrayLayer(0)
                                                    .setLayerCount(1)); 
                                        return vk::SharedImageView(
                                            m_logical_device->createImageView(image_view_create_info),
                                            m_logical_device); }) |
                                  std::ranges::to<std::vector>();
    }

    void Graphics::create_render_pass()
    {
        bool msaa_enabled = m_settings.msaa > vk::SampleCountFlagBits::e1;
        auto builder = RenderPassBuilder(m_logical_device);
        vk::AttachmentReference resolve_attachment;
        vk::AttachmentReference depth_attachment;
        vk::AttachmentReference color_attachment;
        if (msaa_enabled) // attachment 顺序需要与framebuffer的顺序一致
        {
            resolve_attachment = builder.add_resolve_attachment(m_surface_format.format);
            depth_attachment = builder.add_depth_attachment(vk::su::pickDepthFormat(m_physical_device),
                                                            m_settings.msaa);
            color_attachment = builder.add_color_attachment(m_surface_format.format, m_settings.msaa);
        }
        else
        {
            color_attachment = builder.add_color_attachment(m_surface_format.format);
            depth_attachment = builder.add_depth_attachment(vk::su::pickDepthFormat(m_physical_device));
        }

        builder.add_subpass(
            vk::SubpassDescription()
                .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachments(color_attachment)
                .setPDepthStencilAttachment(&depth_attachment)
                .setPResolveAttachments(msaa_enabled ? &resolve_attachment : nullptr));

        builder
            .add_dependency(
                vk::SubpassDependency()
                    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                    .setDstSubpass(0)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite));
        m_render_pass = builder.make_shared();
        m_render_pass_drawer = {
            m_render_pass,
            {RenderSubpassDrawers{}}};
    }

    void Graphics::create_command_pool()
    {
        m_graphics_command_pool =
            vk::SharedCommandPool{
                m_logical_device->createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_queue_family_index}),
                m_logical_device};
        m_transfer_command_pool =
            vk::SharedCommandPool{
                m_logical_device->createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_transfer_queue_family_index}),
                m_logical_device};
    }

    void Graphics::create_framebuffers()
    {
        bool msaa_enabled = m_settings.msaa > vk::SampleCountFlagBits::e1;
        auto depth_builder = DepthStencilAttachment2DBuilder(m_logical_device, m_physical_device)
                                 .set_extent(m_swapchain_extent)
                                 .set_sample_count(m_settings.msaa);
        m_depth_images.clear();
        for (auto _ : std::views::iota(0u, m_swapchain_image_views.size()))
        {
            m_depth_images.emplace_back(depth_builder.build());
        }
        if (msaa_enabled)
        {
            m_msaa_image_data = ColorAttachment2DBuilder(m_logical_device, m_physical_device)
                                    .set_extent(m_swapchain_extent)
                                    .set_sample_count(m_settings.msaa)
                                    .set_usage(vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment)
                                    .build();
        }

        m_framebuffers =
            std::views::zip_transform([this, msaa_enabled](vk::SharedImageView image_view, DeviceImage &depth_image)
                                      {
                                                        vk::FramebufferCreateInfo frame_buffer_info;
                                                        frame_buffer_info.renderPass = m_render_pass.get();
                                                        std::vector<vk::ImageView> attachments;
                                                        if (msaa_enabled)
                                                        {
                                                            attachments = {image_view.get(), depth_image.image_view.get(),  m_msaa_image_data.image_view.get(), };
                                                            frame_buffer_info.setAttachments(attachments);
                                                        }
                                                        else
                                                        {
                                                            attachments = {image_view.get(), depth_image.image_view.get()};
                                                            frame_buffer_info.setAttachments(attachments);
                                                        }
                                                        frame_buffer_info.width = m_swapchain_extent.width;
                                                        frame_buffer_info.height = m_swapchain_extent.height;
                                                        frame_buffer_info.layers = 1;
                                                        return vk::SharedFramebuffer(
                                                            m_logical_device->createFramebuffer(frame_buffer_info),
                                                            m_logical_device
                                                        ); }, m_swapchain_image_views, m_depth_images) |
            std::ranges::to<std::vector>();

        m_clear_values.clear();
        static vk::ClearColorValue backgraound_color = vk::ClearColorValue(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f);
        if (m_settings.msaa > vk::SampleCountFlagBits::e1)
        {
            m_clear_values.push_back(backgraound_color);
            m_clear_values.push_back(vk::ClearDepthStencilValue(1.0f, 0));
            m_clear_values.push_back(backgraound_color);
        }
        else
        {
            m_clear_values.push_back(backgraound_color);
            m_clear_values.push_back(vk::ClearDepthStencilValue(1.0f, 0));
        }
    }

    void Graphics::create_cpu_frames()
    {
        std::vector<vk::SharedCommandBuffer> command_buffers = vk::shared::allocate_command_buffers(m_graphics_command_pool, static_cast<uint32_t>(m_swapchain_image_views.size()));
        m_cpu_frames = std::views::iota((size_t)0, m_swapchain_image_views.size()) |
                       std::views::transform([this, &command_buffers](size_t index)
                                             { return CPUFrame{
                                                   vk::SharedSemaphore(m_logical_device->createSemaphore({}), m_logical_device),
                                                   vk::SharedSemaphore(m_logical_device->createSemaphore({}), m_logical_device),
                                                   vk::SharedFence(m_logical_device->createFence({vk::FenceCreateFlagBits::eSignaled}), m_logical_device),
                                                   command_buffers[index]}; }) |
                       std::ranges::to<std::vector>();
        m_current_cpu_frame = m_cpu_frames.begin();
    }

    Graphics::~Graphics()
    {
        wait_idle();
    }

    void Graphics::on_draw()
    {
        try
        {
            CPUFrame &cpu_frame = *m_current_cpu_frame;
            check(m_logical_device->waitForFences({cpu_frame.in_flight_fence.get()}, false, std::numeric_limits<uint64_t>::max()));
            m_logical_device->resetFences({cpu_frame.in_flight_fence.get()});

            auto cyclic_next = [](auto &it, auto &container) mutable
            { return std::next(it) == container.end() ? it = container.begin() : ++it; };
            cyclic_next(m_current_cpu_frame, m_cpu_frames);

            auto res = m_logical_device->acquireNextImageKHR(*m_swapchain, std::numeric_limits<uint64_t>::max(), *cpu_frame.image_available_semaphore, nullptr);
            m_current_frame_buffer_index = res.value;

            vk::SharedCommandBuffer command_buffer = cpu_frame.command_buffer;
            command_buffer->reset();
            command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
            m_render_pass_drawer.draw(
                *this,
                cpu_frame.command_buffer.get(),
                m_framebuffers[m_current_frame_buffer_index],
                m_clear_values,
                {{0, 0}, m_swapchain_extent});
            cpu_frame.command_buffer->end();
            submit(m_graphics_queue.get(),
                   {command_buffer.get()},
                   {cpu_frame.image_available_semaphore.get()},
                   {vk::PipelineStageFlagBits::eColorAttachmentOutput},
                   {cpu_frame.render_finished_semaphore.get()},
                   cpu_frame.in_flight_fence.get());

            present(
                m_present_queue.get(),
                {cpu_frame.render_finished_semaphore.get()},
                {m_swapchain.get()},
                m_current_frame_buffer_index);
        }
        catch (vk::OutOfDateKHRError)
        {
            recreate_swapchain();
            for (auto &resize_func : resize_funcs)
            {
                resize_func(m_swapchain_extent.width, m_swapchain_extent.height);
            }
            return;
        }
    }

    vk::PresentModeKHR Graphics::present_mode()
    {
        if (const bool *vsync = std::get_if<bool>(&m_settings.vsync))
        {
            return *vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
        }
        else
        {
            return std::get<vk::PresentModeKHR>(m_settings.vsync);
        }
    }

    void Graphics::set_vsync(const std::variant<bool, vk::PresentModeKHR> &vsync)
    {
        if (m_settings.vsync == vsync)
            return;
        m_settings.vsync = vsync;
        recreate_swapchain();
    }

    void Graphics::set_msaa(const vk::SampleCountFlagBits &msaa)
    {
        if (m_settings.msaa == msaa)
            return;
        m_settings.msaa = msaa;

        wait_idle();
        create_render_pass();
        create_framebuffers();
    }

    void Graphics::recreate_swapchain()
    {
        wait_idle();
        create_surface_and_swapchain();
        create_framebuffers();
    }

    void Graphics::wait_idle() const
    {
        m_logical_device->waitIdle();
    }

    bool Graphics::is_minimized() const
    {
        const vk::SurfaceCapabilitiesKHR &surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(m_swapchain.getSurface().get());
        return surface_capabilities.maxImageExtent.width == 0 || surface_capabilities.maxImageExtent.height == 0;
    }
}
