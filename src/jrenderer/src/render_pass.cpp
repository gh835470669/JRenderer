#include "jrenderer/render_pass.h"

namespace jre
{

    vk::AttachmentReference RenderPassBuilder::add_attachment(vk::AttachmentDescription attachment, vk::ImageLayout ref_layout)
    {
        attachments.emplace_back(attachment);
        return {static_cast<uint32_t>(attachments.size() - 1), ref_layout};
    }

    vk::AttachmentReference RenderPassBuilder::add_color_attachment(vk::Format format, vk::SampleCountFlagBits sample_count)
    {
        return add_attachment(vk::AttachmentDescription(vk::AttachmentDescriptionFlags{},
                                                        format,
                                                        sample_count,
                                                        vk::AttachmentLoadOp::eClear,
                                                        vk::AttachmentStoreOp::eStore,
                                                        vk::AttachmentLoadOp::eDontCare,
                                                        vk::AttachmentStoreOp::eDontCare,
                                                        vk::ImageLayout::eUndefined,
                                                        sample_count > vk::SampleCountFlagBits::e1 ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR),
                              vk::ImageLayout::eColorAttachmentOptimal);
    }

    vk::AttachmentReference RenderPassBuilder::add_depth_attachment(vk::Format format, vk::SampleCountFlagBits sample_count)
    {
        return add_attachment(vk::AttachmentDescription(vk::AttachmentDescriptionFlags{},
                                                        format,
                                                        sample_count,
                                                        vk::AttachmentLoadOp::eClear,
                                                        vk::AttachmentStoreOp::eDontCare,
                                                        vk::AttachmentLoadOp::eDontCare,
                                                        vk::AttachmentStoreOp::eDontCare,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::ImageLayout::eDepthStencilAttachmentOptimal),
                              vk::ImageLayout::eDepthStencilAttachmentOptimal);
    }

    vk::AttachmentReference RenderPassBuilder::add_resolve_attachment(vk::Format format, vk::SampleCountFlagBits sample_count)
    {
        return add_attachment(vk::AttachmentDescription(vk::AttachmentDescriptionFlags{},
                                                        format,
                                                        sample_count,
                                                        vk::AttachmentLoadOp::eDontCare,
                                                        vk::AttachmentStoreOp::eStore,
                                                        vk::AttachmentLoadOp::eDontCare,
                                                        vk::AttachmentStoreOp::eDontCare,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::ImageLayout::ePresentSrcKHR),
                              vk::ImageLayout::eColorAttachmentOptimal);
    }

    vk::RenderPassCreateInfo RenderPassBuilder::create_info()
    {
        vk::RenderPassCreateInfo render_pass_info;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
        render_pass_info.pSubpasses = subpasses.data();
        render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
        render_pass_info.pDependencies = dependencies.data();
        return render_pass_info;
    }

    vk::RenderPass RenderPassBuilder::build()
    {
        vk::Device vk_device;
        try
        {
            vk_device = std::get<vk::Device>(device);
        }
        catch (const std::bad_variant_access &ex)
        {
            vk_device = std::get<vk::SharedDevice>(device).get();
        }
        return vk_device.createRenderPass(create_info());
    }

    vk::SharedRenderPass RenderPassBuilder::make_shared()
    {
        vk::SharedDevice vk_device = std::get<vk::SharedDevice>(device);
        return vk::SharedRenderPass(vk_device->createRenderPass(create_info()), vk_device);
    }
}
