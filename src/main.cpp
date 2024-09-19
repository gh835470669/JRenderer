#include <windows.h>
#include <iostream>
#include "app.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_vulkan.h"

// Data
static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int                      g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static VkPhysicalDevice SetupVulkan_SelectPhysicalDevice()
{
    uint32_t gpu_count;
    VkResult err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
    check_vk_result(err);
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus.Data);
    check_vk_result(err);

    // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
    // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
    // dedicated GPUs) is out of scope of this sample.
    for (VkPhysicalDevice& device : gpus)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    // Use first GPU (Integrated) is a Discrete one is not available.
    if (gpu_count > 0)
        return gpus[0];
    return VK_NULL_HANDLE;
}

static void SetupVulkan(ImVector<const char*> instance_extensions, App &app)
{
    VulkanPipeline& pipeline = app.renderer.pipeline;

    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        // err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        g_Instance = pipeline.instance;
        check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select Physical Device (GPU)
    // g_PhysicalDevice = SetupVulkan_SelectPhysicalDevice();
    g_PhysicalDevice = pipeline.physical_device;

    // Select graphics queue family
    {
        // uint32_t count;
        // vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
        // VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
        // vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
        // for (uint32_t i = 0; i < count; i++)
        //     if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        //     {
        //         g_QueueFamily = i;
        //         break;
        //     }
        // free(queues);
        // IM_ASSERT(g_QueueFamily != (uint32_t)-1);

        g_QueueFamily = pipeline.m_graphics_queue_family;
    }

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        // err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        // check_vk_result(err);
        // vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
        g_Device = pipeline.device;
        g_Queue = pipeline.m_graphics_queue;
    }

    // Create Descriptor Pool
    // The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
    // If you wish to load e.g. additional textures you may need to alter pools sizes.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkResult err;

    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}


static void handleMouseMove(int32_t x, int32_t y)
{
	int32_t dx = (int32_t)ImguiHelper::mousePos.x - x;
	int32_t dy = (int32_t)ImguiHelper::mousePos.y - y;

	bool handled = false;

    ImGuiIO& io = ImGui::GetIO();
    handled = io.WantCaptureMouse;
	if (handled) {
		ImguiHelper::mousePos = ImVec2((float)x, (float)y);
		return;
	}

	// if (mouseButtons.left) {
	// 	camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
	// 	viewUpdated = true;
	// }
	// if (mouseButtons.right) {
	// 	camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
	// 	viewUpdated = true;
	// }
	// if (mouseButtons.middle) {
	// 	camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
	// 	viewUpdated = true;
	// }
	ImguiHelper::mousePos = ImVec2((float)x, (float)y);
}

static LRESULT WindowProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(h, msg, wp, lp))
        return true;
    /* More info at [2] */
    switch(msg)
    {
        /*                                                              */
        /* Add a win32 push button and do something when it's clicked.  */
        /* Google will help you with the other widget types.  There are */
        /* many tutorials.                                              */
        /*                                                              */
        // case WM_CREATE: {
        //     HWND hbutton=CreateWindow("BUTTON","Hey There",  /* class and title */
        //                               WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON, /* style */
        //                               0,0,100,30,            /* position */
        //                               h,                     /* parent */
        //                               (HMENU)101,            /* unique (within the application) integer identifier */
        //                               GetModuleHandle(0),0   /* GetModuleHandle(0) gets the hinst */
        //                               );
        // } break;
        case WM_LBUTTONDOWN:
            ImguiHelper::mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
            ImguiHelper::mouseButtons.left = true;
		break;
        case WM_RBUTTONDOWN:
            ImguiHelper::mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
            ImguiHelper::mouseButtons.right = true;
            break;
        case WM_MBUTTONDOWN:
            ImguiHelper::mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
            ImguiHelper::mouseButtons.middle = true;
            break;
        case WM_LBUTTONUP:
            ImguiHelper::mouseButtons.left = false;
            break;
        case WM_RBUTTONUP:
            ImguiHelper::mouseButtons.right = false;
            break;
        case WM_MBUTTONUP:
            ImguiHelper::mouseButtons.middle = false;
            break;
        case WM_MOUSEMOVE:
        {
            handleMouseMove(LOWORD(lp), HIWORD(lp));
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wp)) {
                case 101: /* the unique identifier used above. These are usually #define's */
                    PostQuitMessage(0);
                    break;
                default:;
            }
        } break;


        /*                                 */
        /* Minimally need the cases below: */
        /*                                 */
        case WM_CLOSE: PostQuitMessage(0); break;
        default: 
            return DefWindowProc(h,msg,wp,lp);
    }
    return 0;
}

int WinMain(HINSTANCE hinst,
            HINSTANCE hprev,
            LPSTR     cmdline,
            int       show) {
    if(!hprev){
        WNDCLASS c={0};
        c.lpfnWndProc=WindowProc;
        c.hInstance=hinst;
        c.hIcon=LoadIcon(0,IDI_APPLICATION);
        c.hCursor=LoadCursor(0,IDC_ARROW);
        c.hbrBackground=static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
        c.lpszClassName="MainWindow";
        RegisterClass(&c);
    }



    HWND h=CreateWindow("MainWindow",          /* window class name*/
                   "JRenderer",              /* title  */
                   WS_OVERLAPPEDWINDOW,        /* style */
                   CW_USEDEFAULT,CW_USEDEFAULT,/* position */
                   1920,1080,/* size */
                   0,                          /* parent */
                   0,                          /* menu */
                   hinst,                      
                   0                           /* lparam -- crazytown */
                   );
    App app(hinst, h);
    
    // ImVector<const char*> extensions;
    // extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    // extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    // SetupVulkan(extensions, app);
    
    // ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

    // Create Window Surface
    // VkSurfaceKHR surface;
    // VkWin32SurfaceCreateInfoKHR surface_create_info;
    // surface_create_info.hinstance = hinst;
    // surface_create_info.hwnd = h;
    // surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    // VkResult err = vkCreateWin32SurfaceKHR(g_Instance, &surface_create_info, nullptr, &surface);
    // check_vk_result(err);
    // SetupVulkanWindow(wd, surface, 1920, 1080);

    // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
    // ImGui_ImplWin32_Init(h);
    
    // Setup Platform/Renderer backends
    // ImGui_ImplVulkan_InitInfo init_info = {};
    // init_info.Instance = g_Instance;
    // init_info.PhysicalDevice = g_PhysicalDevice;
    // init_info.Device = g_Device;
    // init_info.QueueFamily = g_QueueFamily;
    // init_info.Queue = g_Queue;
    // init_info.PipelineCache = g_PipelineCache;
    // init_info.DescriptorPool = g_DescriptorPool;
    // init_info.RenderPass = wd->RenderPass;
    // init_info.Subpass = 0;
    // init_info.MinImageCount = g_MinImageCount;
    // init_info.ImageCount = wd->ImageCount;
    // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    // init_info.Allocator = g_Allocator;
    // init_info.CheckVkResultFn = check_vk_result;
    // ImGui_ImplVulkan_Init(&init_info);

    
    ShowWindow(h,show);

    // AllocConsole();
    // freopen("CONOUT$", "w", stdout);

    while(1) {  /* or while(running) */
        MSG msg;
        while(PeekMessage(&msg,0,0,0,PM_REMOVE)) { /* See Note 3,4 */
            if(msg.message==WM_QUIT)
                return (int)msg.wParam;
            std::cout << "PeekMessage: " << msg.message << std::endl;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        
        // (Your code process and dispatch Win32 messages)
        // Start the Dear ImGui frame
        // ImGui_ImplVulkan_NewFrame();
        // ImGui_ImplWin32_NewFrame();
        // ImGui::NewFrame();
        // ImGui::ShowDemoWindow(); // Show demo window! :)     

        app.main_loop();

        // Rendering
        // (Your code clears your framebuffer, renders your other stuff etc.)
        // ImGui::Render();
        // ImDrawData* draw_data = ImGui::GetDrawData();
        // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        // if (!is_minimized)
        // {
        //     wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
        //     wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
        //     wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
        //     wd->ClearValue.color.float32[3] = clear_color.w;
        //     FrameRender(wd, draw_data);
        //     FramePresent(wd);
        // }
        // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData());
        // (Your code calls swapchain's Present() function)
  
    }

    // VK_ERROR_NATIVE_WINDOW_IN_USE_KHR

    // ImGui_ImplVulkan_Shutdown();
    // ImGui_ImplWin32_Shutdown();
    // ImGui::DestroyContext();

    // FreeConsole();
    return 0;
}

/* NOTES
 1. when googling for api docs, etc. prefix everything with msdn.  e.g: msdn winmain
 2. prefer msdn docs to stack overflow for most things.
    When in doubt look for Raymond Chen's blog "The Old New Thing."

 3. Overview of the event loop is at [1].
 4. This event-loop spins when no events are available, consuming all available cpu.
    Also see GetMessage, which blocks until the next message is received.

    Use PeekMessage when you want your application loop to do something while not recieving
    events from the message queue.  You would do that work after the while(PeekMessage(...))
    block.
*/
/* REFERENCES
  [1] : https://msdn.microsoft.com/en-us/library/windows/desktop/ms644927%28v=vs.85%29.aspx
  [2]: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633570(v=vs.85).aspx#designing_proc
*/