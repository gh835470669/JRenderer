#include "jrenderer/surface.h"
#include "jrenderer/physical_device.h"
#include "jrenderer/instance.h"

namespace jre
{
    Surface::Surface(gsl::not_null<const Instance *> instance, gsl::not_null<const PhysicalDevice *> physical_device, HINSTANCE hinst, HWND hwnd) : m_instance(instance), m_physical_device(physical_device)
    {
        vk::Win32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.hinstance = hinst;
        surface_create_info.hwnd = hwnd;
        surface_create_info.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
        vk::Result res = instance->instance().createWin32SurfaceKHR(&surface_create_info, nullptr, &m_surface);
        vk::detail::resultCheck(res, "m_instance.createWin32SurfaceKHR");

        for (auto &format : physical_device->physical_device().getSurfaceFormatsKHR(m_surface))
        {
            if (format.format == vk::Format::eR8G8B8A8Unorm)
            {
                m_surface_format = format;
                break;
            }
        }
        assert(m_surface_format.format != vk::Format::eUndefined);
    }

    Surface::~Surface()
    {
        m_instance->instance().destroySurfaceKHR(m_surface);
    }

    vk::SurfaceCapabilitiesKHR Surface::surface_capabilities() const
    {
        return m_physical_device->physical_device().getSurfaceCapabilitiesKHR(m_surface);
    }
}
