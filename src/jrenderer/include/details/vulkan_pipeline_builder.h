#pragma once

#include "vulkan_pipeline.h"

namespace jre
{
    class VulkanPipelineSwapChainBuilder
    {
    private:
        /* data */
    public:
        VulkanPipelineSwapChainBuilder() : VulkanPipelineSwapChainBuilder(0, 0, false) {};
        VulkanPipelineSwapChainBuilder(int width_, int height_, bool vsync_) : width(width_), height(height_), vsync(vsync_) {};
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
        VulkanPipelineBuilder(HINSTANCE hinst_, HWND hwnd_) : VulkanPipelineBuilder(hinst_, hwnd_, VulkanPipelineSwapChainBuilder()) {};
        VulkanPipelineBuilder(HINSTANCE hinst_, HWND hwnd_, VulkanPipelineSwapChainBuilder swap_chain_builder) : hinst(hinst_), hwnd(hwnd_), swap_chain_builder(swap_chain_builder) {};

        HINSTANCE hinst = nullptr;
        HWND hwnd = nullptr;

        inline VulkanPipelineBuilder &SetWindow(HINSTANCE hinst, HWND hwnd)
        {
            this->hinst = hinst;
            this->hwnd = hwnd;
            return *this;
        }

        std::unique_ptr<VulkanPipeline> Build();

        VulkanPipelineSwapChainBuilder swap_chain_builder;
    };

}