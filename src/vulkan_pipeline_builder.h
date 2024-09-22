#pragma once

#include "vulkan_pipeline.h"

namespace jre
{
    class VulkanPipelineSwapChainBuilder
    {
    private:
        /* data */
    public:
        VulkanPipelineSwapChainBuilder() = default;
        ~VulkanPipelineSwapChainBuilder() = default;

        int width = 0;
        int height = 0;
        bool vsync = false;

        inline VulkanPipelineSwapChainBuilder &SetSize(int width, int height)
        {
            this->width = width;
            this->height = height;
            return *this;
        }
        inline VulkanPipelineSwapChainBuilder &SetVsync(bool vsync)
        {
            this->vsync = vsync;
            return *this;
        }

        vk::SwapchainCreateInfoKHR Build(VulkanPipeline &pipeline);
    };

    class VulkanPipelineBuilder
    {
    public:
        VulkanPipelineBuilder() : VulkanPipelineBuilder(VulkanPipelineSwapChainBuilder()) {};
        VulkanPipelineBuilder(VulkanPipelineSwapChainBuilder swap_chain_builder) : swap_chain_builder(swap_chain_builder) {};

        HINSTANCE hinst = nullptr;
        HWND hwnd = nullptr;

        inline VulkanPipelineBuilder &SetWindow(HINSTANCE hinst, HWND hwnd)
        {
            this->hinst = hinst;
            this->hwnd = hwnd;
            return *this;
        }

        VulkanPipeline Build();
        bool Destroy(VulkanPipeline &pipeline);

        VulkanPipelineSwapChainBuilder swap_chain_builder;
    };

}