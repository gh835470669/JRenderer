#include "jrenderer/instance.h"
#include "jrenderer/physical_device.h"
#include <vulkan/vulkan.hpp>
#include "jrenderer/surface.h"

namespace jre
{
    Instance::~Instance()
    {
        m_instance.destroy();
    }

    Instance::Instance(const InstanceCreateInfo &create_info)
    {
        // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
        // program, which can be useful for layers and tools to provide more debug information.
        vk::ApplicationInfo app_info = vk::ApplicationInfo()
                                           .setPApplicationName(create_info.app_name.c_str())
                                           .setApplicationVersion(create_info.app_version)
                                           .setPEngineName(create_info.engine_name.c_str())
                                           .setEngineVersion(create_info.engine_version)
                                           .setApiVersion(VK_API_VERSION_1_3);

        // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
        // are needed.
        // surface是instance级别的extension，他是管理屏幕的呈现的，确实
        // swap chain 需要用到surface，swap chain 是device级别的extension，确实，是GPU把自己的图像通过swap chain弄到屏幕surface上的
        std::vector<const char *> extensions = {VK_KHR_SURFACE_EXTENSION_NAME};
#if defined(_WIN32)
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

        std::vector<const char *> layers;
#if defined(_DEBUG)
        layers.push_back("VK_LAYER_KHRONOS_validation"); // 检查create_info有哪些数据必须要初始化
#endif
        vk::InstanceCreateInfo vk_create_info = vk::InstanceCreateInfo()
                                                    .setFlags(vk::InstanceCreateFlags())
                                                    .setPApplicationInfo(&app_info)
                                                    .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
                                                    .setPpEnabledExtensionNames(extensions.data())
                                                    .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
                                                    .setPpEnabledLayerNames(layers.data());

        // Create the Vulkan instance.
        m_instance = vk::createInstance(vk_create_info);
    }

    Instance::Instance(const vk::InstanceCreateInfo &create_info)
    {
        // Create the Vulkan instance.
        m_instance = vk::createInstance(create_info);
    }

    std::unique_ptr<PhysicalDevice> Instance::create_first_physical_device() const
    {
        return std::make_unique<PhysicalDevice>(m_instance.enumeratePhysicalDevices().front());
    }

    std::unique_ptr<Surface> Instance::create_surface(gsl::not_null<const PhysicalDevice *> physical_device, HINSTANCE hinst, HWND hwnd) const
    {
        return std::make_unique<Surface>(this, physical_device, hinst, hwnd);
    }

} // namespace jre
