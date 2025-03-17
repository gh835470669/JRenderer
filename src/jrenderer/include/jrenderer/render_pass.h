#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>
#include <vulkan/vulkan_shared.hpp>
#include <variant>

namespace jre
{
    class RenderPassBuilder
    {
    public:
        std::variant<vk::Device, vk::SharedDevice> device;
        std::vector<vk::AttachmentDescription> attachments;
        std::vector<vk::SubpassDescription> subpasses;
        std::vector<vk::SubpassDependency> dependencies;

        RenderPassBuilder(vk::Device device) : device(device) {}
        RenderPassBuilder(vk::SharedDevice device) : device(device) {}
        vk::AttachmentReference add_attachment(vk::AttachmentDescription attachment, vk::ImageLayout ref_layout);
        vk::AttachmentReference add_color_attachment(vk::Format format, vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);
        vk::AttachmentReference add_depth_attachment(vk::Format format, vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);
        vk::AttachmentReference add_resolve_attachment(vk::Format format, vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

        RenderPassBuilder &add_dependency(vk::SubpassDependency dependency)
        {
            dependencies.push_back(dependency);
            return *this;
        }
        RenderPassBuilder &add_subpass(vk::SubpassDescription subpass)
        {
            subpasses.push_back(subpass);
            return *this;
        }

        vk::RenderPass build();
        vk::SharedRenderPass make_shared();

    private:
        vk::RenderPassCreateInfo create_info();
    };
}