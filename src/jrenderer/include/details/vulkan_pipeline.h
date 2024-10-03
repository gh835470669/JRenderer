#pragma once

#include "vulkan/vulkan.hpp"
#include "vulkan_buffer.h"
#include "shader.h"
#include <memory>

namespace jre
{

    class VulkanPipeline
    {
        friend class VulkanPipelineBuilder;

    public:
        VulkanPipeline();
        VulkanPipeline(const VulkanPipeline &) = delete;
        ~VulkanPipeline();

        inline vk::Instance instance() { return m_instance; }
        inline vk::PhysicalDevice physical_device() { return m_physical_device; }
        inline vk::Device device() { return m_device; }
        inline const uint32_t &graphics_queue_family() { return m_graphics_queue_family; }
        inline vk::Queue graphics_queue() { return m_graphics_queue; }
        inline vk::SurfaceKHR surface() { return m_surface; }
        inline const uint32_t &present_queue_family() { return m_present_queue_family; }
        inline vk::Queue present_queue() { return m_present_queue; }
        inline vk::SwapchainKHR swap_chain() { return m_swapchain; }
        inline vk::Extent2D swap_chain_extent() { return m_swap_chain_extent; }
        inline const std::vector<vk::Image> &swap_chain_images() { return m_swap_chain_images; }
        inline vk::CommandPool command_pool() { return m_command_pool; }
        inline vk::CommandBuffer command_buffer() { return m_command_buffer; }
        inline vk::RenderPass render_pass() { return m_render_pass; }
        inline vk::PipelineLayout pipeline_layout() { return m_pipeline_layout; }
        inline vk::Pipeline graphics_pipeline() { return m_graphics_pipeline; }

        class VulkanPipelineDrawContext
        {
        public:
            VulkanPipelineDrawContext(VulkanPipeline &pipeline);
            // noexcept(false) 是为了 外面能够catch vk::OutOfDateKHRError &e。默认是true，如果此时抛出异常就会立马abort
            ~VulkanPipelineDrawContext() noexcept(false);

            VulkanPipeline &m_pipeline;
            uint32_t image_index;

            vk::CommandBuffer &command_buffer() { return m_pipeline.m_command_buffer; }
        };
        VulkanPipelineDrawContext BeginDraw();

        void ReInitSwapChain(vk::SwapchainCreateInfoKHR &swap_chain_create_info);

        VulkanBufferHandle create_buffer(VulkanBufferCreateInfo &buffer_create_info);
        void destroy_buffer(VulkanBufferHandle &buffer);
        VulkanBufferHandle create_buffer_with_memory(VulkanBufferCreateInfo &buffer_create_info, VulkanMemoryCreateInfo &memory_create_info);
        void destroy_buffer_with_memory(VulkanBufferHandle &buffer);
        VulkanMemoryHandle allocate_memory(VulkanBufferHandle &buffer, VulkanMemoryCreateInfo &memory_create_info);
        void free_memory(VulkanMemoryHandle &memory);
        void map_memory(VulkanMemoryHandle &memory, const void *data, size_t size);

    private:
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

        std::vector<vk::Buffer> m_buffers;
        vk::PhysicalDeviceMemoryProperties m_physical_device_memory_properties;
        std::vector<vk::DeviceMemory> m_device_memories;

        void InitVulkan();
        void InitInstance();
        void InitPhysicalDevice();
        void InitDevice();
        void InitSurface(HINSTANCE hinst, HWND hwnd);
        void InitQueueFamily();
        void InitQueue();
        void InitSwapChain(vk::SwapchainCreateInfoKHR &swap_chain_create_info);
        void InitRenderPass();
        void InitPipeline();
        void InitFrameBuffers();
        void InitCommandPool();
        void InitCommandBuffer();
        void InitSyncObjects();

        void DestroySwapChain();

        uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties);
    };

}