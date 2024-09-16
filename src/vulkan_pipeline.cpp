#include "vulkan_pipeline.h"
#include <iostream>
#include <algorithm>
#include <array>

template <class T>
static constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
    return v < lo ? lo : hi < v ? hi
                                : v;
}

VulkanPipeline::VulkanPipeline(HINSTANCE hinst, HWND hwnd)
    : m_win_inst(hinst), m_win_handle(hwnd)
{
    InitVulkan();
}

VulkanPipeline::~VulkanPipeline()
{
    m_device.destroySemaphore(m_image_available_semaphore);
    m_device.destroySemaphore(m_render_finished_semaphore);
    m_device.destroyFence(m_in_flight_fence);
    m_device.destroyShaderModule(m_vertext_shader->m_shader_module);
    m_vertext_shader.reset();
    m_device.destroyShaderModule(m_fragment_shader->m_shader_module);
    m_fragment_shader.reset();
    m_device.destroyPipeline(m_graphics_pipeline);
    m_device.destroyPipelineLayout(m_pipeline_layout);
    m_device.destroyRenderPass(m_render_pass);
    m_device.destroyCommandPool(m_command_pool);
    DestroySwapChain();
    m_device.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroy();
}

void VulkanPipeline::InitVulkan()
{
    InitInstance();
    InitSurface();
    InitPhysicalDevice();
    InitQueueFamily();
    InitDevice();
    InitQueue();
    InitSwapChain();
    InitRenderPass();
    InitPipeline();
    InitFrameBuffers();
    InitCommandPool();
    InitCommandBuffer();
    InitSyncObjects();
}

void VulkanPipeline::InitInstance()
{
    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo app_info = vk::ApplicationInfo()
                                       .setPApplicationName("JRenderer")
                                       .setApplicationVersion(1)
                                       .setPEngineName("JRenderer")
                                       .setEngineVersion(1)
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
    vk::InstanceCreateInfo inst_info = vk::InstanceCreateInfo()
                                           .setFlags(vk::InstanceCreateFlags())
                                           .setPApplicationInfo(&app_info)
                                           .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
                                           .setPpEnabledExtensionNames(extensions.data())
                                           .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
                                           .setPpEnabledLayerNames(layers.data());

    // Create the Vulkan instance.
    m_instance = vk::createInstance(inst_info);
}

void VulkanPipeline::InitPhysicalDevice()
{
    // Pick Physical device
    // Enumerate devices
    std::vector<vk::PhysicalDevice> physic_devices = m_instance.enumeratePhysicalDevices();

    m_physical_device = physic_devices[0];

     // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
    vk::PhysicalDeviceProperties device_properties = m_physical_device.getProperties();
    vk::PhysicalDeviceFeatures device_features = m_physical_device.getFeatures();
    vk::PhysicalDeviceMemoryProperties device_memory_properties = m_physical_device.getMemoryProperties();
}

void VulkanPipeline::InitDevice()
{
    vk::DeviceCreateInfo create_info;
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::array<float, 1> priorities = {1.0f};
    queue_create_infos.push_back(vk::DeviceQueueCreateInfo()
                                     .setQueuePriorities(priorities)
                                     .setQueueCount(1)
                                     .setQueueFamilyIndex(m_graphics_queue_family));

    // Create logic device from physical device
    std::array<const char *, 1> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; // swap chain 需要定义extension，否则会段错误，它应该是dll来的

    create_info.setQueueCreateInfos(queue_create_infos)
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data());
    m_device = m_physical_device.createDevice(create_info);


}

void VulkanPipeline::InitSurface()
{
    // 首先创建Windows 平台的Surface
    vk::Win32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.hinstance = m_win_inst;
    surface_create_info.hwnd = m_win_handle;
    surface_create_info.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
    m_instance.createWin32SurfaceKHR(&surface_create_info, nullptr, &m_surface);
}

void VulkanPipeline::InitQueueFamily()
{
    auto properties = m_physical_device.getQueueFamilyProperties();
    for (int i = 0; i < properties.size(); i++)
    {
        if (m_physical_device.getWin32PresentationSupportKHR(i) && properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            m_graphics_queue_family = i;
            m_present_queue_family = i;
        }
    }
}

void VulkanPipeline::InitQueue()
{
    m_graphics_queue = m_device.getQueue(m_graphics_queue_family, 0);
    m_present_queue = m_device.getQueue(m_present_queue_family, 0);
}

void VulkanPipeline::InitSwapChain()
{
    // Swapchain 和具体的操作系统，窗口系统相关的

    // HINSTANCE hInstance = GetModuleHandle(NULL); // 获取当前应用程序实例的句柄
    // HWND hWnd = GetDesktopWindow(); // 获取桌面窗口句柄
    // 这就是依赖于全局变量了，Non-locality !
    RECT rect;
    GetWindowRect(m_win_handle, &rect);      // 获取窗口大小
    int win_width = rect.right - rect.left;  // 计算窗口宽度
    int win_height = rect.bottom - rect.top; // 计算窗口高度

    // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
    // Frame Buffer and Swapchain
    // a queue of images to present on the screen
    vk::SwapchainCreateInfoKHR swap_chain_create_info;
    swap_chain_create_info
        .setImageSharingMode(vk::SharingMode::eExclusive)            // graphics queue and present queue 是否并行还是互斥，如果是只有一个队列就没关系了
        .setQueueFamilyIndices({m_present_queue_family})                // 使用队列
        .setPresentMode(vk::PresentModeKHR::eFifo)                   // 使用FIFO模式
        .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity) // 设置到屏幕前进行变换：不变
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)   // 不透明
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)     // 颜色附件 GPU才能写
        .setImageArrayLayers(1)                                      // 一张图
        .setClipped(true)                                            // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
        .setSurface(m_surface);

    for (auto &format : m_physical_device.getSurfaceFormatsKHR(m_surface))
    {
        if (format.format == vk::Format::eR8G8B8A8Srgb)
        {
            swap_chain_create_info
                .setImageFormat(format.format)
                .setImageColorSpace(format.colorSpace);
            m_swap_chain_image_format = format.format;
            break;
        }
    }

    auto surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);

    // However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum:
    // 这个会让CPU等？得留意一下不同数量的图会对性能有什么影响
    // 我的电脑最少2张，最多8张耶
    uint32_t image_count = clamp<uint32_t>(surface_capabilities.minImageCount + 1,
                                           surface_capabilities.minImageCount,
                                           surface_capabilities.maxImageCount);
    swap_chain_create_info.setMinImageCount(image_count);

    m_swap_chain_extent = vk::Extent2D(
        clamp<uint32_t>(win_width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
        clamp<uint32_t>(win_height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height));
    swap_chain_create_info.setImageExtent(m_swap_chain_extent); // 窗口大小

    m_swapchain = m_device.createSwapchainKHR(swap_chain_create_info);

    // Get SwapChain Images and Create Image Views
    m_swap_chain_images = m_device.getSwapchainImagesKHR(m_swapchain);
    m_swap_chain_image_views.reserve(m_swap_chain_images.size());
    for (size_t i = 0; i < m_swap_chain_images.size(); i++)
    {
        vk::ImageViewCreateInfo image_view_create_info;
        image_view_create_info
            .setImage(m_swap_chain_images[i])
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(m_swap_chain_image_format)
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
        m_swap_chain_image_views.push_back(m_device.createImageView(image_view_create_info));
    }
}

void VulkanPipeline::InitRenderPass()
{
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
    // framebuffer attachments
    // how many color and depth buffers there
    // how many samples to use for each of them
    // how their contents should be handled throughout the rendering operations

    vk::AttachmentDescription color_attachment;
    color_attachment.format = m_swap_chain_image_format;
    color_attachment.samples = vk::SampleCountFlagBits::e1; // not doing anything with multisampling yet, so we'll stick to 1 sample

    // The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering. We have the following choices for loadOp:
    // VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
    // VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
    // VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear; // clear the framebuffer to black

    // VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
    // VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore; // seeing the rendered triangle on the screen

    // The loadOp and storeOp apply to color and depth data
    // stencilLoadOp / stencilStoreOp apply to stencil data
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    // Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory can change based on what you're trying to do with an image.
    // 内存的布局可以优化
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;   // specifies which layout the image will have before the render pass begins.  we don't care what previous layout the image was in, because color_attachment.loadOp = vk::AttachmentLoadOp::eClear;  clear the framebuffer to black
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR; // specifies the layout to automatically transition to when the render pass finishes

    // [Subpasses]
    // A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes, for example a sequence of post-processing effects that are applied one after another. If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations and conserve memory bandwidth for possibly better performance.

    // Every subpass references one or more of the attachments
    vk::AttachmentReference color_attachment_ref;
    color_attachment_ref.attachment = 0;                                    // specifies which attachment to reference by its index in the attachment descriptions array. Our array consists of a single VkAttachmentDescription, so its index is 0
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal; // specifies which layout we would like the attachment to have during a subpass that uses this reference. Vulkan will automatically transition the attachment to this layout when the subpass is started.

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
    // The following other types of attachments can be referenced by a subpass:

    // pInputAttachments: Attachments that are read from a shader
    // pResolveAttachments: Attachments used for multisampling color attachments
    // pDepthStencilAttachment: Attachment for depth and stencil data
    // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved

    // [Subpass dependencies]
    // Subpass dependencies control the order in which subpasses are executed.
    // The subpass dependencies control the order in which subpasses are executed.
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    m_render_pass = m_device.createRenderPass(render_pass_info);
}

void VulkanPipeline::InitPipeline()
{
    // Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
    // Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    // Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
    // Render pass: the attachments referenced by the pipeline stages and their usage

    // https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html
    // The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't happen until the graphics pipeline is created.
    // That means that we're allowed to destroy the shader modules again as soon as pipeline creation is finished, which is why we'll make them local variables in the createGraphicsPipeline function instead of class members:
    m_vertext_shader = std::make_shared<Shader>("res/shaders/test_vert.spv", m_device);
    m_fragment_shader = std::make_shared<Shader>("res/shaders/test_frag.spv", m_device);

    vk::PipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
    vert_shader_stage_create_info.setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(m_vertext_shader->m_shader_module)
        .setPName("main"); // entrypoint  可以一个模块（文件）就定义vert和frag，指定不同的入口就行

    // There is one more (optional) member, pSpecializationInfo, which we won't be using here, but is worth discussing. It allows you to specify values for shader constants. You can use a single shader module where its behavior can be configured at pipeline creation by specifying different values for the constants used in it. This is more efficient than configuring the shader using variables at render time, because the compiler can do optimizations like eliminating if statements that depend on these values. If you don't have any constants like that, then you can set the member to nullptr, which our struct initialization does automatically.
    // Shader constant，可以优化shader的变量，可以减少if判断

    vk::PipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(m_fragment_shader->m_shader_module)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_create_info, frag_shader_stage_create_info};

    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    // baked into an immutable pipeline state object
    // as a static part of the pipeline or as a dynamic state set in the command buffer.
    // When opting for dynamic viewport(s) and scissor rectangle(s) you need to enable the respective dynamic states for the pipeline:
    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};

    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    //  Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the VkPipelineViewportStateCreateInfo struct.
    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed to the vertex shader. It describes this in roughly two ways:
    // Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
    // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr; // Optional
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr; // Optional

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
    // VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
    // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = vk::False; // 不懂

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swap_chain_extent.width;
    viewport.height = (float)m_swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // window size
    // swap chain size
    // viewport size

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = m_swap_chain_extent;

    // The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader.
    // It also performs 【depth testing】, 【face culling】 and 【the scissor test】,
    // and it can be configured to output fragments that fill entire polygons or just the edges (【wireframe】 rendering).
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature.
    rasterizer.depthClampEnable = vk::False;
    // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. This basically【[disables any output to the framebuffer】.
    rasterizer.rasterizerDiscardEnable = vk::False;
    // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
    // VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
    // VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    // The lineWidth member is straightforward, it describes the thickness of lines in terms of number of fragments. The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;

    // The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's slope. This is sometimes used for shadow mapping,
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    // GPU multisampling： disable
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    // depth/stencil testing
    // f you are using a depth and/or stencil buffer, then you also need to configure the depth and stencil tests using VkPipelineDepthStencilStateCreateInfo. We don't have one right now, so we can simply pass a nullptr instead of a pointer to such a struct. We'll get back to it in the depth buffering chapter.

    // Color blending
    // After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending and there are two ways to do it:
    // 1.  Mix the old and new value to produce a final color
    // 2.  Combine the old and new value using a bitwise operation
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;  // Optional
    color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
    color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;             // Optional
    color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;  // Optional
    color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
    color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;             // Optional

    // 操作伪代码
    // if (blendEnable) {
    // finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    // finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
    // } else {
    //    finalColor = newColor;
    //}
    // finalColor = finalColor & colorWriteMask;

    // 【alpha blending】
    // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
    // finalColor.a = newAlpha.a;
    // This can be accomplished with the following parameters:

    // colorBlendAttachment.blendEnable = VK_TRUE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // The second structure references the array of structures for all of the framebuffers and allows you to set blend constants that you can use as blend factors in the aforementioned calculations.
    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eCopy; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.setLayoutCount = 0;            // Optional
    pipeline_layout_info.pSetLayouts = nullptr;         // Optional
    pipeline_layout_info.pushConstantRangeCount = 0;    // Optional
    pipeline_layout_info.pPushConstantRanges = nullptr; // Optional
    m_pipeline_layout = m_device.createPipelineLayout(pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = m_render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = nullptr; // create a new graphics pipeline by deriving from an existing pipeline
    pipeline_info.basePipelineIndex = -1;
    auto res_value = m_device.createGraphicsPipeline(nullptr, pipeline_info);
    if (res_value.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    m_graphics_pipeline = res_value.value;
}

void VulkanPipeline::InitFrameBuffers()
{
    m_frame_buffers.reserve(m_swap_chain_image_views.size());
    for (size_t i = 0; i < m_swap_chain_image_views.size(); i++)
    {
        vk::ImageView attachments[] = {
            m_swap_chain_image_views[i]};

        vk::FramebufferCreateInfo frame_buffer_info;
        frame_buffer_info.renderPass = m_render_pass;
        frame_buffer_info.attachmentCount = 1;
        frame_buffer_info.pAttachments = attachments;
        frame_buffer_info.width = m_swap_chain_extent.width;
        frame_buffer_info.height = m_swap_chain_extent.height;
        frame_buffer_info.layers = 1;

        m_frame_buffers.push_back(m_device.createFramebuffer(frame_buffer_info));
    }
}

void VulkanPipeline::InitCommandPool()
{
    // We have to create a command pool before we can create command buffers. Command pools manage the memory that is used to store the buffers and command buffers are allocated from them
    vk::CommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.queueFamilyIndex = m_graphics_queue_family;
    cmd_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
    m_command_pool = m_device.createCommandPool(cmd_pool_create_info);

    // for draw command or memory transfer
    // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers
    // Command Buffer vs directly function
    // 1. avalibale together, totaly optimize
    // 2. multithread
    // createCommandBuffers();
}

void VulkanPipeline::InitCommandBuffer()
{
    vk::CommandBufferAllocateInfo cmd_buffer_allocate_info;
    cmd_buffer_allocate_info.commandPool = m_command_pool;
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
    // 没懂...
    cmd_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
    cmd_buffer_allocate_info.commandBufferCount = 1;
    m_command_buffer = m_device.allocateCommandBuffers(cmd_buffer_allocate_info)[0];
}

void VulkanPipeline::InitSyncObjects()
{
    // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
    // Outline of a frame
    // 1. Wait for the previous frame to finish
    // 2. Acquire an image from the swap chain
    // 3. Record a command buffer which draws the scene onto that image
    // 4. Submit the recorded command buffer
    // 5. Present the swap chain image

    // Synchronization  https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Synchronization
    // 程序员需要显式地指定哪些步骤是需要同步的，需要按顺序地执行的，需要你执行完我才执行的
    // (explicit)显式地指定同步规则，是Vulkan的设计原则之一
    // synchronization primitives 同步原语，是我们拥有告诉GPU怎么同步的指令
    // 1. Semaphores https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Semaphores 信号量/锁
    //  signal 表示我完成了，等待这个的线程可以执行了， wait 表示我还没执行，等待这个信号量signal后才执行
    // Semaphore 是让GPU同步的，CPU不会阻塞。下面的Fences是CPU同步的，CPU会阻塞。
    // 2. Fences https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Fences
    //  if the host needs to know when the GPU has finished something, we use a fence.

    // 下面这些步骤是需要同步的
    // 1. Acquire an image from the swap chain
    // 2. Execute commands that draw onto the acquired image
    // 3. Present that image to the screen for presentation, returning it to the swapchain

    vk::SemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.flags = vk::SemaphoreCreateFlags();
    m_image_available_semaphore = m_device.createSemaphore(semaphore_create_info);
    m_render_finished_semaphore = m_device.createSemaphore(semaphore_create_info);
    vk::FenceCreateInfo fence_create_info;
    fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
    m_in_flight_fence = m_device.createFence(fence_create_info);
}

void VulkanPipeline::HandleResize(vk::Result res)
{
    if (res == vk::Result::eErrorOutOfDateKHR)
    {
        // 如果swapchain的extent发生变化，需要重新创建swapchain
        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Resizing
    }
}

void VulkanPipeline::ReInitSwapChain()
{
    // 等待这个逻辑设备host的所有队列的的任务完成
    // 相当于所有队列调用vkQueueWaitIdle
    m_device.waitIdle();  //    https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
    DestroySwapChain();
    InitSwapChain();
    InitFrameBuffers();
}

void VulkanPipeline::DestroySwapChain()
{
    for (auto &frame_buffer : m_frame_buffers)
    {
        m_device.destroyFramebuffer(frame_buffer);
    }
    m_frame_buffers.clear();
    for (auto &image_view : m_swap_chain_image_views)
    {
        m_device.destroyImageView(image_view);
    }
    m_swap_chain_image_views.clear();
    m_device.destroySwapchainKHR(m_swapchain);
}

void VulkanPipeline::draw()
{
    // timeout UINT64_MAX 为不会超时
    // 可以等几个fence一起signal才执行，第二个true表示所有的都signal才会执行,false表示有就执行
    m_device.waitForFences({m_in_flight_fence}, true, UINT64_MAX);  //  wait until the previous frame has finished
    m_device.resetFences({m_in_flight_fence});  // reset the fence to the unsignaled state

    // 会等m_image_available_semaphore signal了，然后再执行
    auto res = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_image_available_semaphore, nullptr);
    uint32_t image_index = res.value;  // m_swap_chain_images的index

    // record command buffer

    //  https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkResetCommandBuffer.html
    m_command_buffer.reset();  // Reset a command buffer to the initial state , vs recording or pending executable state

    vk::CommandBufferBeginInfo cmd_buffer_begin_info;
    // cmd_buffer_begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution.
    
    // The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers.
    cmd_buffer_begin_info.pInheritanceInfo = nullptr;
    m_command_buffer.begin(cmd_buffer_begin_info);  //  begin recording a command buffer

    vk::RenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.renderPass = m_render_pass;
    render_pass_begin_info.framebuffer = m_frame_buffers[image_index];
    render_pass_begin_info.renderArea.offset = vk::Offset2D{0, 0};
    render_pass_begin_info.renderArea.extent = m_swap_chain_extent;

    vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;  // color_attachment.loadOp = vk::AttachmentLoadOp::eClear

    // VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers.
    m_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline);

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swap_chain_extent.width);
    viewport.height = static_cast<float>(m_swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_command_buffer.setViewport(0, viewport);

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = m_swap_chain_extent;
    m_command_buffer.setScissor(0, scissor);

    // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
    // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
    // firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
    // firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
    m_command_buffer.draw(3, 1, 0, 0); 

    m_command_buffer.endRenderPass();

    m_command_buffer.end();

    // submit command buffer

    // 我们希望在ColorAttachmentOutput这个阶段等m_image_available_semaphore好了才继续执行
    // That means that theoretically the implementation can already start executing our vertex shader and such while the image is not yet available.
    // 然后好了以后把m_render_finished_semaphore signal了
    vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::Semaphore wait_semaphore[] = {m_image_available_semaphore};  
    vk::Semaphore signal_semaphore[] = {m_render_finished_semaphore};

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffer;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphore;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphore;

    m_graphics_queue.submit(submit_info, m_in_flight_fence);
    // This allows us to know when it is safe for the command buffer to be reused,

    vk::PresentInfoKHR present_info;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &image_index;
    try {
        m_present_queue.presentKHR(present_info);
    } catch (vk::OutOfDateKHRError) {
        ReInitSwapChain();
    }

    // 1. resize the window会出现crash  Result::eErrorOutOfDateKHR
    // when the swapchain becomes invalid due to window resize, acquiring or presenting an image will result in returncode VK_ERROR_OUT_OF_DATE_KHR
}
