#include "jrenderer/swap_chain.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/surface.h"
#include <algorithm>
#include <fmt/core.h>

namespace jre
{
    SwapChain::SwapChain(gsl::not_null<const LogicalDevice *> logical_device, const Surface &surface, std::unique_ptr<SwapChain> old_swapchain, SwapChainCreateInfo create_info) : m_logical_device(logical_device)
    {

        // Swapchain 和具体的操作系统，窗口系统相关的

        const vk::SurfaceCapabilitiesKHR &surface_capabilities = surface.surface_capabilities();

        uint32_t extend_width = std::clamp<uint32_t>(create_info.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        uint32_t extend_height = std::clamp<uint32_t>(create_info.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
        fmt::print("win_width: {}, win_height: {}\n", create_info.width, create_info.height);
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
        const uint32_t queue_family_index = logical_device->present_queue_family();

        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eMailbox;
        try
        {
            present_mode = std::get<bool>(create_info.vsync) ? vk::PresentModeKHR::eFifoRelaxed : vk::PresentModeKHR::eMailbox;
        }
        catch (const std::bad_variant_access &)
        {
            present_mode = std::get<vk::PresentModeKHR>(create_info.vsync);
        }

        swap_chain_create_info
            .setOldSwapchain(old_swapchain ? old_swapchain->m_swapchain : nullptr)
            .setImageSharingMode(vk::SharingMode::eExclusive)            // graphics queue and present queue 是否并行还是互斥，如果是只有一个队列就没关系了
            .setQueueFamilyIndices({queue_family_index})                 // 使用队列
            .setPresentMode(present_mode)                                // vsync 相关的
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity) // 设置到屏幕前进行变换：不变
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)   // 不透明
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)     // 颜色附件 GPU才能写
            .setImageArrayLayers(1)                                      // 一张图
            .setClipped(true)                                            // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
            .setSurface(surface)
            .setImageFormat(surface.surface_format().format)
            .setImageColorSpace(surface.surface_format().colorSpace);

        // However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum:
        // 这个会让CPU等？得留意一下不同数量的图会对性能有什么影响
        // 我的电脑最少2张，最多8张耶
        uint32_t image_count = std::clamp<uint32_t>(surface_capabilities.minImageCount,
                                                    surface_capabilities.minImageCount,
                                                    surface_capabilities.maxImageCount);
        swap_chain_create_info.setMinImageCount(image_count);

        swap_chain_create_info.setImageExtent(vk::Extent2D(extend_width, extend_height)); // 窗口大小

        m_swapchain = logical_device->device().createSwapchainKHR(swap_chain_create_info);

        m_extent = swap_chain_create_info.imageExtent;
        m_image_format = swap_chain_create_info.imageFormat;

        // Get SwapChain Images and Create Image Views
        m_images = logical_device->device().getSwapchainImagesKHR(m_swapchain);
        m_image_views.reserve(m_images.size());
        for (size_t i = 0; i < m_images.size(); i++)
        {
            vk::ImageViewCreateInfo image_view_create_info;
            image_view_create_info
                .setImage(m_images[i])
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(m_image_format)
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
            m_image_views.push_back(logical_device->device().createImageView(image_view_create_info));
        }
    }

    SwapChain::SwapChain(SwapChain &&other)
        : m_logical_device(std::move(other.m_logical_device)),
          m_swapchain(std::move(other.m_swapchain)),
          m_image_format(std::move(other.m_image_format)),
          m_extent(std::move(other.m_extent)),
          m_images(std::move(other.m_images)),
          m_image_views(std::move(other.m_image_views))
    {
        other.m_swapchain = nullptr;
    }

    SwapChain &SwapChain::operator=(SwapChain &&other)
    {
        swap(other, *this);
        return *this;
    }

    SwapChain::~SwapChain()

    {
        for (auto &image_view : m_image_views)
        {
            m_logical_device->device().destroyImageView(image_view);
        }
        m_image_views.clear();
        m_logical_device->device().destroySwapchainKHR(m_swapchain);
    }

    void SwapChain::recreate(const Surface &surface, SwapChainCreateInfo create_info)
    {
        *this = std::move(
            SwapChain(
                m_logical_device,
                surface,
                std::move(std::make_unique<SwapChain>(std::move(*this))),
                create_info));
        // swap(SwapChain(m_logical_device, surface, std::move(std::make_unique<SwapChain>(std::move(*this))), create_info), *this);
    }
}
