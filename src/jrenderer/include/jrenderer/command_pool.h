#pragma once

#include <vulkan/vulkan.hpp>
#include <gsl/pointers>

namespace jre
{
    class LogicalDevice;
    class CommandBuffer;

    using CommandPoolCreateInfo = vk::CommandPoolCreateInfo;

    class CommandPool
    {
    private:
        gsl::not_null<const LogicalDevice *> m_device;
        vk::CommandPool m_command_pool;

    public:
        CommandPool() = default;
        CommandPool(gsl::not_null<const LogicalDevice *> logical_device, CommandPoolCreateInfo create_Info);
        CommandPool(const CommandPool &) = delete;            // non-copyable
        CommandPool &operator=(const CommandPool &) = delete; // non-copyable
        CommandPool(CommandPool &&) = default;                // movable
        CommandPool &operator=(CommandPool &&) = default;     // movable
        ~CommandPool();

        const vk::CommandPool &command_pool() const { return m_command_pool; }
        operator vk::CommandPool() const { return m_command_pool; }

        std::unique_ptr<CommandBuffer> allocate_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    };
}