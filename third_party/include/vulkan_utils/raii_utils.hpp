#pragma once

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
// unknown compiler... just ignore the warnings for yourselves ;)
#endif

#include "utils.hpp"

#include <numeric>
#include <vulkan/vulkan_raii.hpp>

namespace vk
{
  namespace raii
  {
    namespace su
    {
      vk::raii::DeviceMemory allocateDeviceMemory(vk::raii::Device const &device,
                                                  vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                                                  vk::MemoryRequirements const &memoryRequirements,
                                                  vk::MemoryPropertyFlags memoryPropertyFlags);

      template <typename T>
      void copyToDevice(vk::raii::DeviceMemory const &deviceMemory, T const *pData, size_t count, vk::DeviceSize stride = sizeof(T))
      {
        assert(sizeof(T) <= stride);
        uint8_t *deviceData = static_cast<uint8_t *>(deviceMemory.mapMemory(0, count * stride));
        if (stride == sizeof(T))
        {
          memcpy(deviceData, pData, count * sizeof(T));
        }
        else
        {
          for (size_t i = 0; i < count; i++)
          {
            memcpy(deviceData, &pData[i], sizeof(T));
            deviceData += stride;
          }
        }
        deviceMemory.unmapMemory();
      }

      template <typename T>
      void copyToDevice(vk::raii::DeviceMemory const &deviceMemory, T const &data)
      {
        copyToDevice<T>(deviceMemory, &data, 1);
      }

      template <typename T, class... Args>
      std::unique_ptr<T> make_unique(Args &&...args)
      {
#if (14 <= VULKAN_HPP_CPP_VERSION)
        return std::make_unique<T>(std::forward<Args>(args)...);
#else
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
#endif
      }

      template <typename Func>
      void oneTimeSubmit(vk::raii::Device const &device, vk::raii::CommandPool const &commandPool, vk::raii::Queue const &queue, Func const &func)
      {
        vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, {*commandPool, vk::CommandBufferLevel::ePrimary, 1}).front());
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        func(commandBuffer);
        commandBuffer.end();
        vk::SubmitInfo submitInfo(nullptr, nullptr, *commandBuffer);
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
      }

      void setImageLayout(
          vk::raii::CommandBuffer const &commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout);

      struct BufferData
      {
        BufferData(vk::raii::PhysicalDevice const &physicalDevice,
                   vk::raii::Device const &device,
                   vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
            : buffer(device, vk::BufferCreateInfo({}, size, usage))
#if !defined(NDEBUG)
              ,
              m_size(size), m_usage(usage), m_propertyFlags(propertyFlags)
#endif
        {
          deviceMemory = vk::raii::su::allocateDeviceMemory(device, physicalDevice.getMemoryProperties(), buffer.getMemoryRequirements(), propertyFlags);
          buffer.bindMemory(deviceMemory, 0);
        }

        BufferData(std::nullptr_t) {}

        template <typename DataType>
        void upload(DataType const &data) const
        {
          assert((m_propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) && (m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));
          assert(sizeof(DataType) <= m_size);

          void *dataPtr = deviceMemory.mapMemory(0, sizeof(DataType));
          memcpy(dataPtr, &data, sizeof(DataType));
          deviceMemory.unmapMemory();
        }

        template <typename DataType>
        void upload(std::vector<DataType> const &data, size_t stride = 0) const
        {
          assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);

          size_t elementSize = stride ? stride : sizeof(DataType);
          assert(sizeof(DataType) <= elementSize);

          copyToDevice(deviceMemory, data.data(), data.size(), elementSize);
        }

        template <typename DataType>
        void upload(vk::raii::PhysicalDevice const &physicalDevice,
                    vk::raii::Device const &device,
                    vk::raii::CommandPool const &commandPool,
                    vk::raii::Queue const &queue,
                    std::vector<DataType> const &data,
                    size_t stride) const
        {
          assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
          assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);

          size_t elementSize = stride ? stride : sizeof(DataType);
          assert(sizeof(DataType) <= elementSize);

          size_t dataSize = data.size() * elementSize;
          assert(dataSize <= m_size);

          vk::raii::su::BufferData stagingBuffer(physicalDevice, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
          copyToDevice(stagingBuffer.deviceMemory, data.data(), data.size(), elementSize);

          vk::raii::su::oneTimeSubmit(device,
                                      commandPool,
                                      queue,
                                      [&](vk::raii::CommandBuffer const &commandBuffer)
                                      { commandBuffer.copyBuffer(*stagingBuffer.buffer, *this->buffer, vk::BufferCopy(0, 0, dataSize)); });
        }

        // the DeviceMemory should be destroyed before the Buffer it is bound to; to get that order with the standard destructor
        // of the BufferData, the order of DeviceMemory and Buffer here matters
        vk::raii::DeviceMemory deviceMemory = nullptr;
        vk::raii::Buffer buffer = nullptr;
#if !defined(NDEBUG)
      private:
        vk::DeviceSize m_size;
        vk::BufferUsageFlags m_usage;
        vk::MemoryPropertyFlags m_propertyFlags;
#endif
      };

      struct ImageData
      {
        ImageData(vk::raii::PhysicalDevice const &physicalDevice,
                  vk::raii::Device const &device,
                  vk::Format format_,
                  vk::Extent2D const &extent,
                  vk::ImageTiling tiling,
                  vk::ImageUsageFlags usage,
                  vk::ImageLayout initialLayout,
                  vk::MemoryPropertyFlags memoryProperties,
                  vk::ImageAspectFlags aspectMask)
            : format(format_), image(device,
                                     {vk::ImageCreateFlags(),
                                      vk::ImageType::e2D,
                                      format,
                                      vk::Extent3D(extent, 1),
                                      1,
                                      1,
                                      vk::SampleCountFlagBits::e1,
                                      tiling,
                                      usage | vk::ImageUsageFlagBits::eSampled,
                                      vk::SharingMode::eExclusive,
                                      {},
                                      initialLayout})
        {
          deviceMemory = vk::raii::su::allocateDeviceMemory(device, physicalDevice.getMemoryProperties(), image.getMemoryRequirements(), memoryProperties);
          image.bindMemory(deviceMemory, 0);
          imageView = vk::raii::ImageView(device, vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, {}, {aspectMask, 0, 1, 0, 1}));
        }

        ImageData(std::nullptr_t) {}

        // the DeviceMemory should be destroyed before the Image it is bound to; to get that order with the standard destructor
        // of the ImageData, the order of DeviceMemory and Image here matters
        vk::Format format;
        vk::raii::DeviceMemory deviceMemory = nullptr;
        vk::raii::Image image = nullptr;
        vk::raii::ImageView imageView = nullptr;
      };

      struct DepthBufferData : public ImageData
      {
        DepthBufferData(vk::raii::PhysicalDevice const &physicalDevice, vk::raii::Device const &device, vk::Format format, vk::Extent2D const &extent)
            : ImageData(physicalDevice,
                        device,
                        format,
                        extent,
                        vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        vk::ImageLayout::eUndefined,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        vk::ImageAspectFlagBits::eDepth)
        {
        }
      };

      struct SwapChainData
      {
        SwapChainData(vk::raii::PhysicalDevice const &physicalDevice,
                      vk::raii::Device const &device,
                      vk::raii::SurfaceKHR const &surface,
                      vk::Extent2D const &extent,
                      vk::ImageUsageFlags usage,
                      vk::raii::SwapchainKHR const *pOldSwapchain,
                      uint32_t graphicsQueueFamilyIndex,
                      uint32_t presentQueueFamilyIndex)
        {
          vk::SurfaceFormatKHR surfaceFormat = vk::su::pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
          colorFormat = surfaceFormat.format;

          vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
          vk::Extent2D swapchainExtent;
          if (surfaceCapabilities.currentExtent.width == (std::numeric_limits<uint32_t>::max)())
          {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swapchainExtent.width = vk::su::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
            swapchainExtent.height = vk::su::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
          }
          else
          {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent = surfaceCapabilities.currentExtent;
          }
          vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
                                                             ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                             : surfaceCapabilities.currentTransform;
          vk::CompositeAlphaFlagBitsKHR compositeAlpha =
              (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
              : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
              : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)        ? vk::CompositeAlphaFlagBitsKHR::eInherit
                                                                                                               : vk::CompositeAlphaFlagBitsKHR::eOpaque;
          vk::PresentModeKHR presentMode = vk::su::pickPresentMode(physicalDevice.getSurfacePresentModesKHR(surface));
          vk::SwapchainCreateInfoKHR swapChainCreateInfo({},
                                                         surface,
                                                         vk::su::clamp(3u, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
                                                         colorFormat,
                                                         surfaceFormat.colorSpace,
                                                         swapchainExtent,
                                                         1,
                                                         usage,
                                                         vk::SharingMode::eExclusive,
                                                         {},
                                                         preTransform,
                                                         compositeAlpha,
                                                         presentMode,
                                                         true,
                                                         pOldSwapchain ? **pOldSwapchain : nullptr);
          if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
          {
            uint32_t queueFamilyIndices[2] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
            // If the graphics and present queues are from different queue families, we either have to explicitly
            // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
            // as vk::SharingMode::eConcurrent
            swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
          }
          swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);

          images = swapChain.getImages();

          imageViews.reserve(images.size());
          vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, colorFormat, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
          for (auto image : images)
          {
            imageViewCreateInfo.image = image;
            imageViews.emplace_back(device, imageViewCreateInfo);
          }
        }

        vk::Format colorFormat;
        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> images;
        std::vector<vk::raii::ImageView> imageViews;
      };

      struct TextureData
      {
        TextureData(vk::raii::PhysicalDevice const &physicalDevice,
                    vk::raii::Device const &device,
                    vk::Extent2D const &extent_ = {256, 256},
                    vk::ImageUsageFlags usageFlags = {},
                    vk::FormatFeatureFlags formatFeatureFlags = {},
                    bool anisotropyEnable = false,
                    bool forceStaging = false)
            : format(vk::Format::eR8G8B8A8Unorm), extent(extent_), sampler(device,
                                                                           {{},
                                                                            vk::Filter::eLinear,
                                                                            vk::Filter::eLinear,
                                                                            vk::SamplerMipmapMode::eLinear,
                                                                            vk::SamplerAddressMode::eRepeat,
                                                                            vk::SamplerAddressMode::eRepeat,
                                                                            vk::SamplerAddressMode::eRepeat,
                                                                            0.0f,
                                                                            anisotropyEnable,
                                                                            16.0f,
                                                                            false,
                                                                            vk::CompareOp::eNever,
                                                                            0.0f,
                                                                            0.0f,
                                                                            vk::BorderColor::eFloatOpaqueBlack})
        {
          vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);

          formatFeatureFlags |= vk::FormatFeatureFlagBits::eSampledImage;
          needsStaging = forceStaging || ((formatProperties.linearTilingFeatures & formatFeatureFlags) != formatFeatureFlags);
          vk::ImageTiling imageTiling;
          vk::ImageLayout initialLayout;
          vk::MemoryPropertyFlags requirements;
          if (needsStaging)
          {
            assert((formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
            stagingBufferData = BufferData(physicalDevice, device, extent.width * extent.height * 4, vk::BufferUsageFlagBits::eTransferSrc);
            imageTiling = vk::ImageTiling::eOptimal;
            usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
            initialLayout = vk::ImageLayout::eUndefined;
          }
          else
          {
            imageTiling = vk::ImageTiling::eLinear;
            initialLayout = vk::ImageLayout::ePreinitialized;
            requirements = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
          }
          imageData = ImageData(physicalDevice,
                                device,
                                format,
                                extent,
                                imageTiling,
                                usageFlags | vk::ImageUsageFlagBits::eSampled,
                                initialLayout,
                                requirements,
                                vk::ImageAspectFlagBits::eColor);
        }

        template <typename ImageGenerator>
        void setImage(vk::raii::CommandBuffer const &commandBuffer, ImageGenerator const &imageGenerator)
        {
          void *data = needsStaging ? stagingBufferData.deviceMemory.mapMemory(0, stagingBufferData.buffer.getMemoryRequirements().size)
                                    : imageData.deviceMemory.mapMemory(0, imageData.image.getMemoryRequirements().size);
          imageGenerator(data, extent);
          needsStaging ? stagingBufferData.deviceMemory.unmapMemory() : imageData.deviceMemory.unmapMemory();

          if (needsStaging)
          {
            // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
            vk::raii::su::setImageLayout(commandBuffer, imageData.image, imageData.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            vk::BufferImageCopy copyRegion(0,
                                           extent.width,
                                           extent.height,
                                           vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                           vk::Offset3D(0, 0, 0),
                                           vk::Extent3D(extent, 1));
            commandBuffer.copyBufferToImage(stagingBufferData.buffer, imageData.image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
            // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
            vk::raii::su::setImageLayout(
                commandBuffer, imageData.image, imageData.format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
          }
          else
          {
            // If we can use the linear tiled image as a texture, just do it
            vk::raii::su::setImageLayout(
                commandBuffer, imageData.image, imageData.format, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
          }
        }

        vk::Format format;
        vk::Extent2D extent;
        bool needsStaging;
        BufferData stagingBufferData = nullptr;
        ImageData imageData = nullptr;
        vk::raii::Sampler sampler;
      };

      std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const &physicalDevice,
                                                                           vk::raii::SurfaceKHR const &surface);

      vk::raii::CommandBuffer makeCommandBuffer(vk::raii::Device const &device, vk::raii::CommandPool const &commandPool);

      void fullPipelineBarrier(vk::raii::CommandBuffer const &commandBuffer);

      vk::raii::DescriptorPool makeDescriptorPool(vk::raii::Device const &device, std::vector<vk::DescriptorPoolSize> const &poolSizes);

      vk::raii::DescriptorSetLayout makeDescriptorSetLayout(vk::raii::Device const &device,
                                                            std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> const &bindingData,
                                                            vk::DescriptorSetLayoutCreateFlags flags = {});

      vk::raii::Device makeDevice(vk::raii::PhysicalDevice const &physicalDevice,
                                  uint32_t queueFamilyIndex,
                                  std::vector<std::string> const &extensions = {},
                                  vk::PhysicalDeviceFeatures const *physicalDeviceFeatures = nullptr,
                                  void const *pNext = nullptr);

      std::vector<vk::raii::Framebuffer> makeFramebuffers(vk::raii::Device const &device,
                                                          vk::raii::RenderPass &renderPass,
                                                          std::vector<vk::raii::ImageView> const &imageViews,
                                                          vk::raii::ImageView const *pDepthImageView,
                                                          vk::Extent2D const &extent);

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
                                              vk::raii::RenderPass const &renderPass);

      vk::raii::Image makeImage(vk::raii::Device const &device);

      vk::raii::Instance makeInstance(vk::raii::Context const &context,
                                      std::string const &appName,
                                      std::string const &engineName,
                                      std::vector<std::string> const &layers = {},
                                      std::vector<std::string> const &extensions = {},
                                      uint32_t apiVersion = VK_API_VERSION_1_0);

      vk::raii::RenderPass makeRenderPass(vk::raii::Device const &device,
                                          vk::Format colorFormat,
                                          vk::Format depthFormat,
                                          vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
                                          vk::ImageLayout colorFinalLayout = vk::ImageLayout::ePresentSrcKHR);

      vk::Format pickDepthFormat(vk::raii::PhysicalDevice const &physicalDevice);

      void submitAndWait(vk::raii::Device const &device, vk::raii::Queue const &queue, vk::raii::CommandBuffer const &commandBuffer);

      void updateDescriptorSets(
          vk::raii::Device const &device,
          vk::raii::DescriptorSet const &descriptorSet,
          std::vector<std::tuple<vk::DescriptorType, vk::raii::Buffer const &, vk::DeviceSize, vk::raii::BufferView const *>> const &bufferData,
          vk::raii::su::TextureData const &textureData,
          uint32_t bindingOffset = 0);

      void updateDescriptorSets(
          vk::raii::Device const &device,
          vk::raii::DescriptorSet const &descriptorSet,
          std::vector<std::tuple<vk::DescriptorType, vk::raii::Buffer const &, vk::DeviceSize, vk::raii::BufferView const *>> const &bufferData,
          std::vector<vk::raii::su::TextureData> const &textureData,
          uint32_t bindingOffset = 0);

    } // namespace su
  } // namespace raii
} // namespace vk
