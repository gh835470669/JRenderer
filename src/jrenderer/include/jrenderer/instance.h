#pragma once

#include <vulkan/vulkan.hpp>
#include <string>
#include <gsl/pointers>

namespace jre
{
    struct InstanceCreateInfo
    {
        std::string app_name = "";
        uint32_t app_version = 1;
        std::string engine_name = "";
        uint32_t engine_version = 1;
    };

    class PhysicalDevice;
    class Surface;
    class Instance
    {
    private:
        vk::Instance m_instance;

    public:
        Instance(const InstanceCreateInfo &create_info = {});
        Instance(const vk::InstanceCreateInfo &create_info);
        Instance(const Instance &) = delete; // non-copyable
        Instance &operator=(const Instance &) = delete;
        ~Instance();

        inline const vk::Instance instance() const noexcept { return m_instance; }
        operator vk::Instance() const noexcept { return m_instance; }

        std::unique_ptr<PhysicalDevice> create_first_physical_device() const;
        std::unique_ptr<Surface> create_surface(gsl::not_null<const PhysicalDevice *> physical_device, HINSTANCE hinst, HWND hwnd) const;
    };

}
