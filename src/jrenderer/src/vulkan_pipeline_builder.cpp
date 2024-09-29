#include "details/vulkan_pipeline_builder.h"
#include <fmt/core.h>
namespace jre
{
    template <class T>
    static constexpr const T &clamp(const T &v, const T &lo, const T &hi)
    {
        return v < lo ? lo : hi < v ? hi
                                    : v;
    }

    std::unique_ptr<VulkanPipeline> VulkanPipelineBuilder::Build()
    {
        std::unique_ptr<VulkanPipeline> pipeline = std::make_unique<VulkanPipeline>();
        pipeline->InitInstance();
        assert(hinst && hwnd);
        pipeline->InitSurface(hinst, hwnd);
        pipeline->InitPhysicalDevice();
        pipeline->InitQueueFamily();
        pipeline->InitDevice();
        pipeline->InitQueue();
        vk::SwapchainCreateInfoKHR swap_chain_create_info = swap_chain_builder.Build(*pipeline);
        pipeline->InitSwapChain(swap_chain_create_info);
        pipeline->InitRenderPass();
        pipeline->InitPipeline();
        pipeline->InitFrameBuffers();
        pipeline->InitCommandPool();
        pipeline->InitCommandBuffer();
        pipeline->InitSyncObjects();
        return std::move(pipeline);
    }

    vk::SwapchainCreateInfoKHR VulkanPipelineSwapChainBuilder::Build(VulkanPipeline &pipeline)
    {
        // Swapchain 和具体的操作系统，窗口系统相关的
        vk::SurfaceKHR surface = pipeline.surface();
        vk::PhysicalDevice physical_device = pipeline.physical_device();
        vk::Device device = pipeline.device();
        vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);

        uint32_t extend_width = 0;
        uint32_t extend_height = 0;
        // HINSTANCE hInstance = GetModuleHandle(NULL); // 获取当前应用程序实例的句柄
        // HWND hWnd = GetDesktopWindow(); // 获取桌面窗口句柄
        // 这就是依赖于全局变量了，Non-locality !
        // RECT rect;
        // GetWindowRect(m_win_handle, &rect);      // 获取窗口大小
        // int win_width = rect.right - rect.left;  // 计算窗口宽度
        // int win_height = rect.bottom - rect.top; // 计算窗口高度

        extend_width = clamp<uint32_t>(width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        extend_height = clamp<uint32_t>(height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

        fmt::print("win_width: {}, win_height: {}\n", width, height);
        fmt::print("surface_capabilities minImageExtent: {}, {}\n", surface_capabilities.minImageExtent.width, surface_capabilities.minImageExtent.height);
        fmt::print("surface_capabilities maxImageExtent: {}, {}\n", surface_capabilities.maxImageExtent.width, surface_capabilities.maxImageExtent.height);
        fmt::print("clamped current size: {}, {}\n", extend_width, extend_height);
        // 【window minimize】最小化时值会是这样的
        // win_width: 160, win_height: 28
        // surface_capabilities minImageExtent: 0, 0
        // surface_capabilities maxImageExtent: 0, 0
        // 然后validation layer 会报提醒 extend的长宽不能为0，倒是不会崩，暂时不处理了

        // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
        // Frame Buffer and Swapchain
        // a queue of images to present on the screen
        vk::SwapchainCreateInfoKHR swap_chain_create_info;
        swap_chain_create_info
            .setImageSharingMode(vk::SharingMode::eExclusive)                                 // graphics queue and present queue 是否并行还是互斥，如果是只有一个队列就没关系了
            .setQueueFamilyIndices({pipeline.present_queue_family()})                         // 使用队列
            .setPresentMode(vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox) // 使用FIFO模式
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)                      // 设置到屏幕前进行变换：不变
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)                        // 不透明
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)                          // 颜色附件 GPU才能写
            .setImageArrayLayers(1)                                                           // 一张图
            .setClipped(true)                                                                 // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
            .setSurface(surface);

        for (auto &format : physical_device.getSurfaceFormatsKHR(surface))
        {
            if (format.format == vk::Format::eR8G8B8A8Srgb)
            {
                swap_chain_create_info
                    .setImageFormat(format.format)
                    .setImageColorSpace(format.colorSpace);
                // m_swap_chain_image_format = format.format;
                break;
            }
        }

        // However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum:
        // 这个会让CPU等？得留意一下不同数量的图会对性能有什么影响
        // 我的电脑最少2张，最多8张耶
        uint32_t image_count = clamp<uint32_t>(surface_capabilities.minImageCount + 1,
                                               surface_capabilities.minImageCount,
                                               surface_capabilities.maxImageCount);
        swap_chain_create_info.setMinImageCount(image_count);

        // m_swap_chain_extent = vk::Extent2D(extend_width, extend_height);
        swap_chain_create_info.setImageExtent(vk::Extent2D(extend_width, extend_height)); // 窗口大小
        return swap_chain_create_info;
    }
}