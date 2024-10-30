#include "jrenderer.h"
#include <iostream>
#include "tracy/Tracy.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_graphics(&window),
                                           m_res_meshes(m_graphics.logical_device(), std::move(m_graphics.command_pool()->allocate_command_buffer())),
                                           m_res_textures(m_graphics.logical_device(), std::move(m_graphics.command_pool()->allocate_command_buffer())),
                                           model(
                                               m_res_meshes.get_mesh("res/model/viking_room/viking_room.obj"),
                                               m_res_textures.get_texture("res/model/viking_room/viking_room.png"),
                                               m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                                 {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}),
                                               std::make_unique<UniformBuffer<UniformBufferObject>>(m_graphics.logical_device())),
                                           model2(
                                               m_res_meshes.get_mesh("res/model/viking_room/viking_room.obj"),
                                               m_res_textures.get_texture("res/model/viking_room/viking_room.png"),
                                               m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                                 {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}),
                                               std::make_unique<UniformBuffer<UniformBufferObject>>(m_graphics.logical_device())),
                                           m_imgui_context(m_window, m_graphics)
    {
        m_window.message_handlers.push_back(m_imgui_context);

        render_set.graphics_pipeline = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                std::make_shared<VertexShader>(m_graphics.logical_device(), "res/shaders/test_vert.spv"),
                std::make_shared<FragmentShader>(m_graphics.logical_device(), "res/shaders/test_frag.spv"),
                Vertex::get_pipeline_vertex_input_state(),
                {model.get_descriptor_set().descriptor_set_layout()},
                {},
                true,
                false});

        render_set.render_objects.push_back(model);
        render_set.render_objects.push_back(model2);
        model.descriptor_set->update_descriptor_sets<UniformBufferObject>({model.uniform_buffer.get()}, {model.texture.get()});
        model2.descriptor_set->update_descriptor_sets<UniformBufferObject>({model2.uniform_buffer.get()}, {model2.texture.get()});

        render_set_renderer.func_get_viewport = viewport::get_full_viewport;
        render_set_renderer.func_get_scissor = scissor::get_full_scissor;
        render_set_renderer.render_set = &render_set;
    }

    JRenderer::~JRenderer()
    {
    }

    void JRenderer::new_imgui_frame()
    {
        m_imgui_context.new_frame();
    }

    void JRenderer::new_frame()
    {
        {
            ZoneScoped;
            tick();
        }
        {
            ZoneScoped;
            m_graphics.draw(
                std::vector<IRenderSetRenderer *>{&render_set_renderer, &m_imgui_context});
            // m_imgui_context.draw(draw_context);
        }
    }

    void JRenderer::tick()
    {

        if (m_graphics.swap_chain()->extent().width == 0 || m_graphics.swap_chain()->extent().height == 0)
            return;
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        model.model_matrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model2.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

} // namespace jre
