#pragma once

#include "VulkanDevice.h"
#include "VulkanUIOverlay.h"

class VulkanExampleBase
{
private:
    /* data */
public:
    VulkanExampleBase(/* args */);
    ~VulkanExampleBase();

    vks::VulkanDevice *vulkanDevice;
    vks::UIOverlay UIOverlay;
    VkDevice device{ VK_NULL_HANDLE };
    std::vector<VkShaderModule> shaderModules;
    std::string title = "Vulkan Example";
    /** @brief Last frame time measured using a high performance timer (if available) */
    float frameTimer = 1.0f;

    // Camera camera;

    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
};


