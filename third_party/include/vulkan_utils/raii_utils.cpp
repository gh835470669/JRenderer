// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#if defined(_MSC_VER)
// no need to ignore any warnings with MSVC
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-braces"
#elif defined(__GNUC__)
// no need to ignore any warnings with GCC
#else
// unknow compiler... just ignore the warnings for yourselves ;)
#endif

#include "utils.hpp"

#include <iomanip>
#include <numeric>
#include <vulkan/vulkan.hpp>
#if defined(VULKAN_HPP_NO_TO_STRING)
#include <vulkan/vulkan_to_string.hpp>
#endif
#include <vulkan/vulkan_static_assertions.hpp>

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace vk
{
  namespace su
  {
    vk::raii::DeviceMemory allocateDeviceMemory(vk::raii::Device const &device,
                                                vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                                                vk::MemoryRequirements const &memoryRequirements,
                                                vk::MemoryPropertyFlags memoryPropertyFlags)
    {
      uint32_t memoryTypeIndex = vk::su::findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags);
      vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, memoryTypeIndex);
      return vk::raii::DeviceMemory(device, memoryAllocateInfo);
    }

    void setImageLayout(
        vk::raii::CommandBuffer const &commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout)
    {
      vk::AccessFlags sourceAccessMask;
      switch (oldImageLayout)
      {
      case vk::ImageLayout::eTransferDstOptimal:
        sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
      case vk::ImageLayout::ePreinitialized:
        sourceAccessMask = vk::AccessFlagBits::eHostWrite;
        break;
      case vk::ImageLayout::eGeneral: // sourceAccessMask is empty
      case vk::ImageLayout::eUndefined:
        break;
      default:
        assert(false);
        break;
      }

      vk::PipelineStageFlags sourceStage;
      switch (oldImageLayout)
      {
      case vk::ImageLayout::eGeneral:
      case vk::ImageLayout::ePreinitialized:
        sourceStage = vk::PipelineStageFlagBits::eHost;
        break;
      case vk::ImageLayout::eTransferDstOptimal:
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        break;
      case vk::ImageLayout::eUndefined:
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        break;
      default:
        assert(false);
        break;
      }

      vk::AccessFlags destinationAccessMask;
      switch (newImageLayout)
      {
      case vk::ImageLayout::eColorAttachmentOptimal:
        destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
      case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        destinationAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
      case vk::ImageLayout::eGeneral: // empty destinationAccessMask
      case vk::ImageLayout::ePresentSrcKHR:
        break;
      case vk::ImageLayout::eShaderReadOnlyOptimal:
        destinationAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
      case vk::ImageLayout::eTransferSrcOptimal:
        destinationAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
      case vk::ImageLayout::eTransferDstOptimal:
        destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
      default:
        assert(false);
        break;
      }

      vk::PipelineStageFlags destinationStage;
      switch (newImageLayout)
      {
      case vk::ImageLayout::eColorAttachmentOptimal:
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        break;
      case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        break;
      case vk::ImageLayout::eGeneral:
        destinationStage = vk::PipelineStageFlagBits::eHost;
        break;
      case vk::ImageLayout::ePresentSrcKHR:
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        break;
      case vk::ImageLayout::eShaderReadOnlyOptimal:
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        break;
      case vk::ImageLayout::eTransferDstOptimal:
      case vk::ImageLayout::eTransferSrcOptimal:
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
        break;
      default:
        assert(false);
        break;
      }

      vk::ImageAspectFlags aspectMask;
      if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
      {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
        {
          aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
      }
      else
      {
        aspectMask = vk::ImageAspectFlagBits::eColor;
      }

      vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, 1, 0, 1);
      vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask,
                                                destinationAccessMask,
                                                oldImageLayout,
                                                newImageLayout,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                image,
                                                imageSubresourceRange);
      return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
    }

    std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const &physicalDevice,
                                                                         vk::raii::SurfaceKHR const &surface)
    {
      std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
      assert(queueFamilyProperties.size() < (std::numeric_limits<uint32_t>::max)());

      uint32_t graphicsQueueFamilyIndex = vk::su::findGraphicsQueueFamilyIndex(queueFamilyProperties);
      if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface))
      {
        return std::make_pair(graphicsQueueFamilyIndex,
                              graphicsQueueFamilyIndex); // the first graphicsQueueFamilyIndex does also support presents
      }

      // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
      // graphics and present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++)
      {
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
        {
          return std::make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));
        }
      }

      // there's nothing like a single family index that supports both graphics and present -> look for an other
      // family index that supports present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++)
      {
        if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
        {
          return std::make_pair(graphicsQueueFamilyIndex, static_cast<uint32_t>(i));
        }
      }

      throw std::runtime_error("Could not find queues for both graphics or present -> terminating");
    }

    vk::raii::CommandBuffer makeCommandBuffer(vk::raii::Device const &device, vk::raii::CommandPool const &commandPool)
    {
      vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
      return std::move(vk::raii::CommandBuffers(device, commandBufferAllocateInfo).front());
    }

    void fullPipelineBarrier(vk::raii::CommandBuffer const &commandBuffer)
    {
      vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
                                      vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite);
      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, memoryBarrier, nullptr, nullptr);
    }

    vk::raii::DescriptorPool makeDescriptorPool(vk::raii::Device const &device, std::vector<vk::DescriptorPoolSize> const &poolSizes)
    {
      assert(!poolSizes.empty());
      uint32_t maxSets = std::accumulate(
          poolSizes.begin(), poolSizes.end(), 0, [](uint32_t sum, vk::DescriptorPoolSize const &dps)
          { return sum + dps.descriptorCount; });
      assert(0 < maxSets);

      vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets, poolSizes);
      return vk::raii::DescriptorPool(device, descriptorPoolCreateInfo);
    }

    vk::raii::DescriptorSetLayout makeDescriptorSetLayout(vk::raii::Device const &device,
                                                          std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> const &bindingData,
                                                          vk::DescriptorSetLayoutCreateFlags flags = {})
    {
      std::vector<vk::DescriptorSetLayoutBinding> bindings(bindingData.size());
      for (size_t i = 0; i < bindingData.size(); i++)
      {
        bindings[i] = vk::DescriptorSetLayoutBinding(
            vk::su::checked_cast<uint32_t>(i), std::get<0>(bindingData[i]), std::get<1>(bindingData[i]), std::get<2>(bindingData[i]));
      }
      vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(flags, bindings);
      return vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
    }

    vk::raii::Device makeDevice(vk::raii::PhysicalDevice const &physicalDevice,
                                uint32_t queueFamilyIndex,
                                std::vector<std::string> const &extensions = {},
                                vk::PhysicalDeviceFeatures const *physicalDeviceFeatures = nullptr,
                                void const *pNext = nullptr)
    {
      std::vector<char const *> enabledExtensions;
      enabledExtensions.reserve(extensions.size());
      for (auto const &ext : extensions)
      {
        enabledExtensions.push_back(ext.data());
      }

      float queuePriority = 0.0f;
      vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority);
      vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), deviceQueueCreateInfo, {}, enabledExtensions, physicalDeviceFeatures, pNext);
      return vk::raii::Device(physicalDevice, deviceCreateInfo);
    }

    std::vector<vk::raii::Framebuffer> makeFramebuffers(vk::raii::Device const &device,
                                                        vk::raii::RenderPass &renderPass,
                                                        std::vector<vk::raii::ImageView> const &imageViews,
                                                        vk::raii::ImageView const *pDepthImageView,
                                                        vk::Extent2D const &extent)
    {
      vk::ImageView attachments[2];
      attachments[1] = pDepthImageView ? *pDepthImageView : vk::ImageView();

      vk::FramebufferCreateInfo framebufferCreateInfo(
          vk::FramebufferCreateFlags(), renderPass, pDepthImageView ? 2 : 1, attachments, extent.width, extent.height, 1);
      std::vector<vk::raii::Framebuffer> framebuffers;
      framebuffers.reserve(imageViews.size());
      for (auto const &imageView : imageViews)
      {
        attachments[0] = imageView;
        framebuffers.push_back(vk::raii::Framebuffer(device, framebufferCreateInfo));
      }

      return framebuffers;
    }

    vk::raii::Pipeline makeGraphicsPipeline(vk::raii::Device const &device,
                                            vk::raii::PipelineCache const &pipelineCache,
                                            vk::raii::ShaderModule const &vertexShaderModule,
                                            vk::SpecializationInfo const *vertexShaderSpecializationInfo,
                                            vk::raii::ShaderModule const &fragmentShaderModule,
                                            vk::SpecializationInfo const *fragmentShaderSpecializationInfo,
                                            uint32_t vertexStride,
                                            std::vector<std::pair<vk::Format, uint32_t>> const &vertexInputAttributeFormatOffset,
                                            vk::FrontFace frontFace,
                                            bool depthBuffered,
                                            vk::raii::PipelineLayout const &pipelineLayout,
                                            vk::raii::RenderPass const &renderPass)
    {
      std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = {
          vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main", vertexShaderSpecializationInfo),
          vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main", fragmentShaderSpecializationInfo)};

      std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
      vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
      vk::VertexInputBindingDescription vertexInputBindingDescription(0, vertexStride);

      if (0 < vertexStride)
      {
        vertexInputAttributeDescriptions.reserve(vertexInputAttributeFormatOffset.size());
        for (uint32_t i = 0; i < vertexInputAttributeFormatOffset.size(); i++)
        {
          vertexInputAttributeDescriptions.emplace_back(i, 0, vertexInputAttributeFormatOffset[i].first, vertexInputAttributeFormatOffset[i].second);
        }
        pipelineVertexInputStateCreateInfo.setVertexBindingDescriptions(vertexInputBindingDescription);
        pipelineVertexInputStateCreateInfo.setVertexAttributeDescriptions(vertexInputAttributeDescriptions);
      }

      vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(),
                                                                                    vk::PrimitiveTopology::eTriangleList);

      vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

      vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlags(),
                                                                                    false,
                                                                                    false,
                                                                                    vk::PolygonMode::eFill,
                                                                                    vk::CullModeFlagBits::eBack,
                                                                                    frontFace,
                                                                                    false,
                                                                                    0.0f,
                                                                                    0.0f,
                                                                                    0.0f,
                                                                                    1.0f);

      vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

      vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
      vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
          vk::PipelineDepthStencilStateCreateFlags(), depthBuffered, depthBuffered, vk::CompareOp::eLessOrEqual, false, false, stencilOpState, stencilOpState);

      vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                                  vk::ColorComponentFlagBits::eA);
      vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(false,
                                                                              vk::BlendFactor::eZero,
                                                                              vk::BlendFactor::eZero,
                                                                              vk::BlendOp::eAdd,
                                                                              vk::BlendFactor::eZero,
                                                                              vk::BlendFactor::eZero,
                                                                              vk::BlendOp::eAdd,
                                                                              colorComponentFlags);
      vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
          vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, pipelineColorBlendAttachmentState, {{1.0f, 1.0f, 1.0f, 1.0f}});

      std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
      vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), dynamicStates);

      vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(vk::PipelineCreateFlags(),
                                                                pipelineShaderStageCreateInfos,
                                                                &pipelineVertexInputStateCreateInfo,
                                                                &pipelineInputAssemblyStateCreateInfo,
                                                                nullptr,
                                                                &pipelineViewportStateCreateInfo,
                                                                &pipelineRasterizationStateCreateInfo,
                                                                &pipelineMultisampleStateCreateInfo,
                                                                &pipelineDepthStencilStateCreateInfo,
                                                                &pipelineColorBlendStateCreateInfo,
                                                                &pipelineDynamicStateCreateInfo,
                                                                pipelineLayout,
                                                                renderPass);

      return vk::raii::Pipeline(device, pipelineCache, graphicsPipelineCreateInfo);
    }

    vk::raii::Image makeImage(vk::raii::Device const &device)
    {
      vk::ImageCreateInfo imageCreateInfo({},
                                          vk::ImageType::e2D,
                                          vk::Format::eB8G8R8A8Unorm,
                                          vk::Extent3D(640, 640, 1),
                                          1,
                                          1,
                                          vk::SampleCountFlagBits::e1,
                                          vk::ImageTiling::eLinear,
                                          vk::ImageUsageFlagBits::eTransferSrc);
      return vk::raii::Image(device, imageCreateInfo);
    }

    vk::raii::Instance makeInstance(vk::raii::Context const &context,
                                    std::string const &appName,
                                    std::string const &engineName,
                                    std::vector<std::string> const &layers = {},
                                    std::vector<std::string> const &extensions = {},
                                    uint32_t apiVersion = VK_API_VERSION_1_0)
    {
      vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, apiVersion);
      std::vector<char const *> enabledLayers = vk::su::gatherLayers(layers
#if !defined(NDEBUG)
                                                                     ,
                                                                     context.enumerateInstanceLayerProperties()
#endif
      );
      std::vector<char const *> enabledExtensions = vk::su::gatherExtensions(extensions
#if !defined(NDEBUG)
                                                                             ,
                                                                             context.enumerateInstanceExtensionProperties()
#endif
      );
#if defined(NDEBUG)
      vk::StructureChain<vk::InstanceCreateInfo>
#else
      vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
          instanceCreateInfoChain = vk::su::makeInstanceCreateInfoChain({}, applicationInfo, enabledLayers, enabledExtensions);

      return vk::raii::Instance(context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
    }

    vk::raii::RenderPass makeRenderPass(vk::raii::Device const &device,
                                        vk::Format colorFormat,
                                        vk::Format depthFormat,
                                        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
                                        vk::ImageLayout colorFinalLayout = vk::ImageLayout::ePresentSrcKHR)
    {
      std::vector<vk::AttachmentDescription> attachmentDescriptions;
      assert(colorFormat != vk::Format::eUndefined);
      attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                          colorFormat,
                                          vk::SampleCountFlagBits::e1,
                                          loadOp,
                                          vk::AttachmentStoreOp::eStore,
                                          vk::AttachmentLoadOp::eDontCare,
                                          vk::AttachmentStoreOp::eDontCare,
                                          vk::ImageLayout::eUndefined,
                                          colorFinalLayout);
      if (depthFormat != vk::Format::eUndefined)
      {
        attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                            depthFormat,
                                            vk::SampleCountFlagBits::e1,
                                            loadOp,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eDepthStencilAttachmentOptimal);
      }
      vk::AttachmentReference colorAttachment(0, vk::ImageLayout::eColorAttachmentOptimal);
      vk::AttachmentReference depthAttachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
      vk::SubpassDescription subpassDescription(vk::SubpassDescriptionFlags(),
                                                vk::PipelineBindPoint::eGraphics,
                                                {},
                                                colorAttachment,
                                                {},
                                                (depthFormat != vk::Format::eUndefined) ? &depthAttachment : nullptr);
      vk::RenderPassCreateInfo renderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpassDescription);
      return vk::raii::RenderPass(device, renderPassCreateInfo);
    }

    vk::Format pickDepthFormat(vk::raii::PhysicalDevice const &physicalDevice)
    {
      std::vector<vk::Format> candidates = {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
      for (vk::Format format : candidates)
      {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
          return format;
        }
      }
      throw std::runtime_error("failed to find supported format!");
    }

    void submitAndWait(vk::raii::Device const &device, vk::raii::Queue const &queue, vk::raii::CommandBuffer const &commandBuffer)
    {
      vk::raii::Fence fence(device, vk::FenceCreateInfo());
      queue.submit(vk::SubmitInfo(nullptr, nullptr, *commandBuffer), fence);
      while (vk::Result::eTimeout == device.waitForFences({fence}, VK_TRUE, vk::su::FenceTimeout))
        ;
    }

    void updateDescriptorSets(
        vk::raii::Device const &device,
        vk::raii::DescriptorSet const &descriptorSet,
        std::vector<std::tuple<vk::DescriptorType, vk::raii::Buffer const &, vk::DeviceSize, vk::raii::BufferView const *>> const &bufferData,
        vk::raii::su::TextureData const &textureData,
        uint32_t bindingOffset = 0)
    {
      std::vector<vk::DescriptorBufferInfo> bufferInfos;
      bufferInfos.reserve(bufferData.size());

      std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
      writeDescriptorSets.reserve(bufferData.size() + 1);
      uint32_t dstBinding = bindingOffset;
      for (auto const &bd : bufferData)
      {
        bufferInfos.emplace_back(std::get<1>(bd), 0, std::get<2>(bd));
        vk::BufferView bufferView;
        if (std::get<3>(bd))
        {
          bufferView = *std::get<3>(bd);
        }
        writeDescriptorSets.emplace_back(
            descriptorSet, dstBinding++, 0, 1, std::get<0>(bd), nullptr, &bufferInfos.back(), std::get<3>(bd) ? &bufferView : nullptr);
      }

      vk::DescriptorImageInfo imageInfo(textureData.sampler, textureData.imageData.imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
      writeDescriptorSets.emplace_back(descriptorSet, dstBinding, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, nullptr, nullptr);

      device.updateDescriptorSets(writeDescriptorSets, nullptr);
    }

    void updateDescriptorSets(
        vk::raii::Device const &device,
        vk::raii::DescriptorSet const &descriptorSet,
        std::vector<std::tuple<vk::DescriptorType, vk::raii::Buffer const &, vk::DeviceSize, vk::raii::BufferView const *>> const &bufferData,
        std::vector<vk::raii::su::TextureData> const &textureData,
        uint32_t bindingOffset = 0)
    {
      std::vector<vk::DescriptorBufferInfo> bufferInfos;
      bufferInfos.reserve(bufferData.size());

      std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
      writeDescriptorSets.reserve(bufferData.size() + (textureData.empty() ? 0 : 1));
      uint32_t dstBinding = bindingOffset;
      for (auto const &bd : bufferData)
      {
        bufferInfos.emplace_back(std::get<1>(bd), 0, std::get<2>(bd));
        vk::BufferView bufferView;
        if (std::get<3>(bd))
        {
          bufferView = *std::get<3>(bd);
        }
        writeDescriptorSets.emplace_back(
            descriptorSet, dstBinding++, 0, 1, std::get<0>(bd), nullptr, &bufferInfos.back(), std::get<3>(bd) ? &bufferView : nullptr);
      }

      std::vector<vk::DescriptorImageInfo> imageInfos;
      if (!textureData.empty())
      {
        imageInfos.reserve(textureData.size());
        for (auto const &thd : textureData)
        {
          imageInfos.emplace_back(thd.sampler, thd.imageData.imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
        writeDescriptorSets.emplace_back(descriptorSet,
                                         dstBinding,
                                         0,
                                         vk::su::checked_cast<uint32_t>(imageInfos.size()),
                                         vk::DescriptorType::eCombinedImageSampler,
                                         imageInfos.data(),
                                         nullptr,
                                         nullptr);
      }

      device.updateDescriptorSets(writeDescriptorSets, nullptr);
    }
  }
}
