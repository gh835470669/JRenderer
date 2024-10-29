#include "jrenderer/render_pass.h"
#include "jrenderer/logical_device.h"

jre::RenderPass::RenderPass(gsl::not_null<const LogicalDevice *> logical_device, vk::Format color_format, vk::Format depth_format) : m_device(logical_device)
{
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
    // framebuffer attachments
    // how many color and depth buffers there
    // how many samples to use for each of them
    // how their contents should be handled throughout the rendering operations

    vk::AttachmentDescription color_attachment;
    color_attachment.format = color_format;
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

    vk::AttachmentDescription depth_attachment;
    depth_attachment.format = depth_format;
    depth_attachment.samples = vk::SampleCountFlagBits::e1;
    depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depth_attachment_ref;
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    // The following other types of attachments can be referenced by a subpass:

    // pInputAttachments: Attachments that are read from a shader
    // pResolveAttachments: Attachments used for multisampling color attachments
    // pDepthStencilAttachment: Attachment for depth and stencil data
    // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved

    std::array<vk::AttachmentDescription, 2> attachments = {color_attachment, depth_attachment};

    // [Subpass dependencies]
    // Subpass dependencies control the order in which subpasses are executed.
    // The subpass dependencies control the order in which subpasses are executed.
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    m_render_pass = m_device->device().createRenderPass(render_pass_info);
}

jre::RenderPass::~RenderPass()
{
    m_device->device().destroyRenderPass(m_render_pass);
}
void jre::RenderPass::begin(vk::CommandBuffer command_buffer, vk::Framebuffer frame_buffer, vk::Rect2D render_area, vk::ClearColorValue color, vk::ClearDepthStencilValue depth_stencil) const
{
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.renderPass = m_render_pass;
    render_pass_info.framebuffer = frame_buffer;
    render_pass_info.renderArea = render_area;

    std::array<vk::ClearValue, 2> clear_values;
    clear_values[0] = color;
    clear_values[1] = depth_stencil;
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
}
void jre::RenderPass::end(vk::CommandBuffer command_buffer) const
{
    command_buffer.endRenderPass();
}