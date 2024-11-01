#pragma once

#include <vulkan/vulkan.hpp>
#include <functional>
#include "jrenderer/mesh.h"
#include "jrenderer/uniform_buffer.h"

namespace jre
{
    class GraphicsPipeline;
    class CommandBuffer;

    namespace viewport
    {
        vk::Viewport get_full_viewport(const vk::Extent2D &win_extent);
    }

    namespace scissor
    {
        vk::Rect2D get_full_scissor(const vk::Extent2D &win_extent);
    }

    template <typename VertexType = Vertex, typename IndexType = uint32_t>
    class IRenderSetObject
    {
    public:
        virtual std::shared_ptr<const Mesh<VertexType, IndexType>> get_mesh() const = 0;
        virtual glm::mat4 get_model_matrix() const = 0;
        virtual ~IRenderSetObject() = default;
    };

    class DescriptorSet;
    class DefaultRenderSet
    {
    public:
        std::shared_ptr<GraphicsPipeline> graphics_pipeline;
        std::shared_ptr<DescriptorSet> descriptor_set;
        std::shared_ptr<UniformBuffer<UniformBufferObject>> uniform_buffer;
        std::vector<std::reference_wrapper<const IRenderSetObject<Vertex, uint32_t>>> render_objects;
        DefaultRenderSet(/* args */) = default;
        ~DefaultRenderSet() = default;
    };

    class Graphics;
    class IRenderSetRenderer
    {
    public:
        virtual void draw(const Graphics &graphics, const CommandBuffer &command_buffer) = 0;
    };

    class SwapChain;
    class DefaultRenderSetRenderer : public IRenderSetRenderer
    {
    public:
        const DefaultRenderSet *render_set;
        std::function<vk::Viewport(const vk::Extent2D &)> func_get_viewport;
        std::function<vk::Rect2D(const vk::Extent2D &)> func_get_scissor;

        void draw(const Graphics &graphics, const CommandBuffer &command_buffer) override;

        void update_uniform_buffer(const SwapChain &swap_chian, const IRenderSetObject<> &render_obj) const;
    };
}
