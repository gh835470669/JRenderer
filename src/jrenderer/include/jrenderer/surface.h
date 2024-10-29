#pragma once

#include <vulkan/vulkan.hpp>
#include <windows.h>
#include <gsl/pointers>

namespace jre
{
    class Instance;
    class PhysicalDevice;
    class Surface
    {
    private:
        gsl::not_null<const Instance *> m_instance;
        gsl::not_null<const PhysicalDevice *> m_physical_device;
        vk::SurfaceKHR m_surface;
        vk::SurfaceFormatKHR m_surface_format;

    public:
        Surface(gsl::not_null<const Instance *> instance, gsl::not_null<const PhysicalDevice *> physical_device, HINSTANCE hinst, HWND hwnd);
        Surface(const Surface &) = delete; // non-copyable
        Surface &operator=(const Surface &) = delete;
        ~Surface();

        inline const vk::SurfaceKHR &surface() const noexcept { return m_surface; }
        operator const vk::SurfaceKHR &() const noexcept { return m_surface; }

        vk::SurfaceCapabilitiesKHR surface_capabilities() const;
        inline const vk::SurfaceFormatKHR &surface_format() const noexcept { return m_surface_format; }
    };
} // namespace jre
