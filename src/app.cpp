#include "app.h"
#include "imgui_helper/VulkanExampleBase.h"
#include "imgui_helper/VulkanDevice.h"

App::App(HINSTANCE hinst, HWND hwnd) : renderer(hinst, hwnd)
{
    auto& pipeline = renderer.pipeline;
    imgui_base.device = pipeline.device;
    imgui_base.vulkanDevice = new vks::VulkanDevice(pipeline.physical_device);
    imgui_base.vulkanDevice->logicalDevice = imgui_base.device;
    imgui_base.vulkanDevice->commandPool = pipeline.m_command_pool;
    imgui = new ImguiHelper(&imgui_base);
    imgui->init((float)1920, (float)1080);
	imgui->initResources(pipeline.m_render_pass, pipeline.m_graphics_queue, "res/shaders/glsl/");
}

App::~App()
{
    delete imgui;
    delete imgui_base.vulkanDevice;
}

template <class T>
static constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
    return v < lo ? lo : hi < v ? hi
                                : v;
}


void App::main_loop()
{
    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    RECT rect;
    GetWindowRect(renderer.pipeline.m_win_handle, &rect);      // 获取窗口大小
    int win_width = rect.right - rect.left;  // 计算窗口宽度
    int win_height = rect.bottom - rect.top; // 计算窗口高度
    vk::SurfaceCapabilitiesKHR surface_capabilities = renderer.pipeline.physical_device.getSurfaceCapabilitiesKHR(renderer.pipeline.surface);
    win_width = clamp<uint32_t>(win_width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    win_height = clamp<uint32_t>(win_height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    
    io.DisplaySize = ImVec2((float)win_width, (float)win_height);
    io.DeltaTime = imgui_base.frameTimer;

    io.MousePos = ImVec2(ImguiHelper::mousePos.x, ImguiHelper::mousePos.y);
    io.MouseDown[0] = ImguiHelper::mouseButtons.left;
    io.MouseDown[1] = ImguiHelper::mouseButtons.right;
    io.MouseDown[2] = ImguiHelper::mouseButtons.middle;

    int frameCounter = 0;
    imgui->newFrame(&imgui_base, (frameCounter == 0));
	imgui->updateBuffers();

    // renderer.main_loop();
    renderer.pipeline.before_draw();
    renderer.pipeline.draw();
    // Render imGui
    if (imgui_base.UIOverlay.visible) {
        imgui->drawFrame(renderer.pipeline.m_command_buffer);
    }
    renderer.pipeline.after_draw();
}