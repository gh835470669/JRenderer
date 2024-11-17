#include "jrenderer/render_pass.h"
#include "jrenderer/logical_device.h"

namespace jre
{

    RenderPass::RenderPass(gsl::not_null<const LogicalDevice *> logical_device, vk::Format color_format, vk::Format depth_format, vk::SampleCountFlagBits sample_count) : m_device(logical_device)
    {
        bool msaa_enabled = sample_count > vk::SampleCountFlagBits::e1;

        // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
        // framebuffer attachments
        // how many color and depth buffers there
        // how many samples to use for each of them
        // how their contents should be handled throughout the rendering operations

        vk::AttachmentDescription color_attachment;
        color_attachment.format = color_format;
        color_attachment.samples = sample_count;

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
        color_attachment.initialLayout = vk::ImageLayout::eUndefined;                                                             // specifies which layout the image will have before the render pass begins.  we don't care what previous layout the image was in, because color_attachment.loadOp = vk::AttachmentLoadOp::eClear;  clear the framebuffer to black
        color_attachment.finalLayout = msaa_enabled ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR; // specifies the layout to automatically transition to when the render pass finishes

        // [Subpasses]
        // A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes, for example a sequence of post-processing effects that are applied one after another. If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations and conserve memory bandwidth for possibly better performance.

        // Every subpass references one or more of the attachments
        vk::AttachmentReference color_attachment_ref;
        color_attachment_ref.attachment = 0;                                    // specifies which attachment to reference by its index in the attachment descriptions array. Our array consists of a single VkAttachmentDescription, so its index is 0
        color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal; // specifies which layout we would like the attachment to have during a subpass that uses this reference. Vulkan will automatically transition the attachment to this layout when the subpass is started.

        vk::AttachmentDescription depth_attachment;
        depth_attachment.format = depth_format;
        depth_attachment.samples = sample_count;
        depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
        depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
        depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depth_attachment_ref;
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentDescription color_attachment_resolve;
        if (msaa_enabled)
        {
            color_attachment_resolve.format = color_format;
            color_attachment_resolve.samples = vk::SampleCountFlagBits::e1;
            color_attachment_resolve.loadOp = vk::AttachmentLoadOp::eDontCare;
            color_attachment_resolve.storeOp = vk::AttachmentStoreOp::eStore;
            color_attachment_resolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            color_attachment_resolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            color_attachment_resolve.initialLayout = vk::ImageLayout::eUndefined;
            color_attachment_resolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        }

        vk::AttachmentReference color_attachment_resolve_ref;
        if (msaa_enabled)
        {
            color_attachment_resolve_ref.attachment = 2;
            color_attachment_resolve_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        }

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
        subpass.pDepthStencilAttachment = &depth_attachment_ref;
        if (msaa_enabled)
        {
            subpass.pResolveAttachments = &color_attachment_resolve_ref;
        }

        // The following other types of attachments can be referenced by a subpass:

        // pInputAttachments: Attachments that are read from a shader
        // pResolveAttachments: Attachments used for multisampling color attachments
        // pDepthStencilAttachment: Attachment for depth and stencil data
        // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved

        std::vector<vk::AttachmentDescription> attachments = {color_attachment, depth_attachment};
        if (msaa_enabled)
        {
            attachments.push_back(color_attachment_resolve);
        }

        // [Subpass dependencies]
        // Subpass dependencies control the order in which subpasses are executed.
        // The subpass dependencies control the order in which subpasses are executed.
        // 如果这个依赖定不好，那就会因为同步等待而浪费性能？！ (下面这两篇问答启蒙了subpass dependency是什么)
        // https://stackoverflow.com/questions/62371266/why-is-a-single-depth-buffer-sufficient-for-this-vulkan-swapchain-render-loop
        // https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/
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

    RenderPass::RenderPass(RenderPass &&other) : m_device(other.m_device),
                                                 m_render_pass(std::exchange(other.m_render_pass, VK_NULL_HANDLE))
    {
    }

    RenderPass &RenderPass::operator=(RenderPass &&other)
    {
        RenderPass tmp(std::move(other));
        std::swap(m_device, tmp.m_device);
        std::swap(m_render_pass, tmp.m_render_pass);
        return *this;
    }

    RenderPass::~RenderPass()
    {
        m_device->device().destroyRenderPass(m_render_pass);
    }
    void RenderPass::begin(vk::CommandBuffer command_buffer, vk::Framebuffer frame_buffer, vk::Rect2D render_area, vk::ClearColorValue color, vk::ClearDepthStencilValue depth_stencil) const
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
    void RenderPass::end(vk::CommandBuffer command_buffer) const
    {
        command_buffer.endRenderPass();
    }
}
