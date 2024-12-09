#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{
  class PhysicalDevice;
  class Surface;
  class SwapChain;
  struct SwapChainCreateInfo;
  class LogicalDevice
  {
  private:
    vk::Device m_device;
    uint32_t m_graphics_queue_family;
    uint32_t m_present_queue_family;
    uint32_t m_transfer_queue_family;
    vk::Queue m_graphics_queue;
    vk::Queue m_present_queue;
    vk::Queue m_transfer_queue;
    gsl::not_null<const PhysicalDevice *> m_physical_device;

  public:
    LogicalDevice() = default;
    LogicalDevice(gsl::not_null<const PhysicalDevice *> physical_device);
    LogicalDevice(const LogicalDevice &) = delete;            // non-copyable
    LogicalDevice &operator=(const LogicalDevice &) = delete; // non-copyable
    LogicalDevice(LogicalDevice &&) = default;                // movable
    LogicalDevice &operator=(LogicalDevice &&) = default;     // movable
    ~LogicalDevice();

    inline const vk::Device &device() const noexcept { return m_device; }
    operator const vk::Device &() const noexcept { return m_device; }

    inline uint32_t graphics_queue_family() const noexcept { return m_graphics_queue_family; }
    inline uint32_t present_queue_family() const noexcept { return m_present_queue_family; }
    inline uint32_t transfer_queue_family() const noexcept { return m_transfer_queue_family; }
    inline const vk::Queue &graphics_queue() const noexcept { return m_graphics_queue; }
    inline const vk::Queue &present_queue() const noexcept { return m_present_queue; }
    inline const vk::Queue &transfer_queue() const noexcept { return m_transfer_queue; }

    inline const PhysicalDevice *physical_device() const noexcept { return m_physical_device; }

    std::unique_ptr<SwapChain> create_swapchain(const Surface &surface, std::unique_ptr<SwapChain> old_swapchian, SwapChainCreateInfo create_info);

    void present(const SwapChain &swap_chain, uint32_t image_index, const std::vector<vk::Semaphore> &wait_semaphores);
  };

}