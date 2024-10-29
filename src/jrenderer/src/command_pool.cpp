#include "jrenderer/command_pool.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/command_buffer.h"

namespace jre
{
    CommandPool::CommandPool(gsl::not_null<const LogicalDevice *> logical_device, CommandPoolCreateInfo create_Info) : m_device(logical_device), m_command_pool(logical_device->device().createCommandPool(create_Info)) {}

    CommandPool::~CommandPool()
    {
        m_device->device().destroyCommandPool(m_command_pool);
    }

    std::unique_ptr<CommandBuffer> CommandPool::allocate_command_buffer(vk::CommandBufferLevel level) const
    {
        return std::make_unique<CommandBuffer>(m_device, this, m_device->device().allocateCommandBuffers({m_command_pool, level, 1})[0]);
    }

}
