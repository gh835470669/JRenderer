#pragma once

#include "vulkan/vulkan.hpp"

#include "shader.h"
#include <memory>

class VulkanPipeline
{
public:
    VulkanPipeline(HINSTANCE hinst, HWND hwnd);
    ~VulkanPipeline();

    void draw();

private:
    HINSTANCE m_win_inst;
    HWND m_win_handle;

    vk::Instance m_instance;
    vk::PhysicalDevice m_physical_device;
    vk::Device m_device;
    uint32_t m_graphics_queue_family;
    vk::Queue m_graphics_queue;
    
    vk::SurfaceKHR m_surface;
    uint32_t m_present_queue_family;
    vk::Queue m_present_queue;
    vk::SwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swap_chain_images;
    std::vector<vk::ImageView> m_swap_chain_image_views;
    vk::Format m_swap_chain_image_format;
    vk::Extent2D m_swap_chain_extent;

    std::shared_ptr<Shader> m_vertext_shader;
    std::shared_ptr<Shader> m_fragment_shader;

    vk::PipelineLayout m_pipeline_layout;
    vk::RenderPass m_render_pass;
    vk::Pipeline m_graphics_pipeline;

    std::vector<vk::Framebuffer> m_frame_buffers;

    vk::CommandPool m_command_pool;
    vk::CommandBuffer m_command_buffer;

    vk::Semaphore m_image_available_semaphore;
    vk::Semaphore m_render_finished_semaphore;
    vk::Fence m_in_flight_fence;

    void InitVulkan();
    void InitInstance();
    void InitDevice();
    void InitSwapChain();
    void InitRenderPass();
    void InitPipeline();
    void InitFrameBuffers();
    void InitCommandPool();
    void InitCommandBuffer();
    void InitSyncObjects();
};
