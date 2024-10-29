#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <variant>

namespace jre
{
    struct SwapChainCreateInfo
    {
        int width = 1;
        int height = 1;
        std::variant<bool, vk::PresentModeKHR> vsync = false;
    };

    class Surface;
    class LogicalDevice;
    class SwapChain
    {
    private:
        gsl::not_null<const LogicalDevice *> m_logical_device;
        vk::SwapchainKHR m_swapchain;
        vk::Format m_image_format;
        vk::Extent2D m_extent;
        std::vector<vk::Image> m_images;
        std::vector<vk::ImageView> m_image_views;

    public:
        SwapChain(gsl::not_null<const LogicalDevice *> logical_device, const Surface &surface, std::unique_ptr<SwapChain> old_swapchain = nullptr, SwapChainCreateInfo create_info = {});
        SwapChain(const SwapChain &) = delete;            // non-copyable
        SwapChain &operator=(const SwapChain &) = delete; // non-copyable
        SwapChain(SwapChain &&);                          // movable
        SwapChain &operator=(SwapChain &&);               // movable
        ~SwapChain();

        inline const vk::SwapchainKHR &swapchain() const noexcept { return m_swapchain; }
        operator const vk::SwapchainKHR &() const noexcept { return m_swapchain; }
        inline vk::Format image_format() const noexcept { return m_image_format; }
        inline vk::Extent2D extent() const noexcept { return m_extent; }
        inline const std::vector<vk::Image> &images() const noexcept { return m_images; }
        inline const std::vector<vk::ImageView> &image_views() const noexcept { return m_image_views; }

        friend void swap(SwapChain &left, SwapChain &right) noexcept
        {
            using std::swap;
            swap(left.m_logical_device, right.m_logical_device);
            swap(left.m_swapchain, right.m_swapchain);
            swap(left.m_image_format, right.m_image_format);
            swap(left.m_extent, right.m_extent);
            swap(left.m_images, right.m_images);
            swap(left.m_image_views, right.m_image_views);
        }

        void recreate(const Surface &surface, SwapChainCreateInfo create_info = {});
    };
}