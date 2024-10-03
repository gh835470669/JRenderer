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
}