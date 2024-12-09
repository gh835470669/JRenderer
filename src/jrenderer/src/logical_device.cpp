#include "jrenderer/logical_device.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/surface.h"
#include "jrenderer/swap_chain.h"
#include <gsl/pointers>
#include <gsl/assert>
#include <map>

namespace jre
{
    LogicalDevice::LogicalDevice(gsl::not_null<const PhysicalDevice *> physical_device) : m_physical_device(physical_device)
    {
        auto properties = physical_device->physical_device().getQueueFamilyProperties();
        bool found_graphics = false;
        bool found_present = false;
        bool found_transfer = false;
        for (int i = 0; i < properties.size(); i++)
        {
            if (properties[i].queueFlags & vk::QueueFlagBits::eTransfer)
            {
                m_transfer_queue_family = i;
                found_transfer = true;
            }
            if (physical_device->physical_device().getWin32PresentationSupportKHR(i) && properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                m_graphics_queue_family = i;
                m_present_queue_family = i;
                found_present = true;
                found_graphics = true;
            }

            if (found_graphics && found_present && found_transfer)
                break;
        }

        std::map<uint32_t, int> unique_queue_families = {{m_graphics_queue_family, 0}, {m_present_queue_family, 0}, {m_transfer_queue_family, 0}};
        unique_queue_families[m_graphics_queue_family]++;
        unique_queue_families[m_present_queue_family]++;
        unique_queue_families[m_transfer_queue_family]++;

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
        std::array<const char *, 1> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; // swap chain 需要定义extension，否则会段错误，它应该是dll来的

        create_info.setQueueCreateInfos(queue_create_infos)
            .setQueueCreateInfoCount(static_cast<uint32_t>(queue_create_infos.size()))
            .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
            .setPpEnabledExtensionNames(extensions.data());
        m_device = physical_device->physical_device().createDevice(create_info);

        m_graphics_queue = m_device.getQueue(m_graphics_queue_family, --unique_queue_families[m_graphics_queue_family]);
        m_present_queue = m_device.getQueue(m_present_queue_family, --unique_queue_families[m_present_queue_family]);
        m_transfer_queue = m_device.getQueue(m_transfer_queue_family, --unique_queue_families[m_transfer_queue_family]);
    }

    LogicalDevice::~LogicalDevice()
    {
        m_device.destroy();
    }

    std::unique_ptr<SwapChain> LogicalDevice::create_swapchain(const Surface &surface, std::unique_ptr<SwapChain> old_swapchian = nullptr, SwapChainCreateInfo create_info = {})
    {
        return std::make_unique<SwapChain>(this, surface, std::move(old_swapchian), create_info);
    }

    void LogicalDevice::present(const SwapChain &swap_chain, uint32_t image_index, const std::vector<vk::Semaphore> &wait_semaphores)
    {
        vk::PresentInfoKHR present_info;
        present_info.setImageIndices(image_index)
            .setSwapchains(swap_chain.swapchain())
            .setWaitSemaphores(wait_semaphores);
        m_present_queue.presentKHR(present_info);
    }
}
