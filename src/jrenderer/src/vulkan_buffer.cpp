#include "details/vulkan_buffer.h"
#include "details/vulkan_pipeline.h"

namespace jre
{
    void VulkanBufferHandle::destroy()
    {
        m_pipeline.destroy_buffer(*this);
    }

    void VulkanBufferHandle::destroy_with_memory()
    {
        m_pipeline.destroy_buffer_with_memory(*this);
    }

    void VulkanBufferHandle::map_memory(const void *data, size_t size)
    {
        m_pipeline.map_memory(m_memory, data, size);
    }
}