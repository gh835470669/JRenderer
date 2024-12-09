#include "jrenderer.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <chrono>
#include "jrenderer/asset/obj_file.h"

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_graphics(&window),
                                           m_imgui_context(m_window, m_graphics),
                                           camera_controller(input_manager),
                                           star_rail_char_render_set(m_graphics.logical_device())
    {
        m_window.message_handlers.push_back(m_imgui_context);

        input_manager.input_manager().SetDisplaySize(m_window.width(), m_window.height());

        camera_controller.default_camera = camera_init();
        camera_controller.default_camera.target_position = {0.0f, 15.0f, 40.0f};
        auto orient = glm::quat(glm::vec3(0, 0, 0));
        camera_controller.default_camera.orientation = {orient.x, orient.y, orient.z, orient.w};
        camera_controller.camera = &camera;
        camera_controller.reset_default_camera();

        model_lingsha = std::move(load_lingsha<Texture2DDynamicMipmaps>(m_graphics,
                                                                        *m_graphics.command_pool()->allocate_command_buffer(), star_rail_char_render_set.ubo.descriptor(), true));
        model_lingsha.model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        star_rail_char_render_set.render_objects.push_back(model_lingsha);
        star_rail_char_render_set.descriptor_set = m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}});
        star_rail_char_render_set.descriptor_set->update_descriptor_sets({star_rail_char_render_set.ubo.descriptor()});
        auto vert_shader = std::make_shared<VertexShader>(m_graphics.logical_device(), "res/shaders/star_rail_vert.spv");
        auto frag_shader = std::make_shared<FragmentShader>(m_graphics.logical_device(), "res/shaders/star_rail_frag.spv");
        std::vector<vk::DescriptorSetLayout> descriptor_sets{star_rail_char_render_set.descriptor_set->descriptor_set_layout(),
                                                             model_lingsha.sub_mesh_materials[0].descriptor_set->descriptor_set_layout()};
        auto body_material_index = std::distance(model_lingsha.model_parts.begin(), std::find(model_lingsha.model_parts.begin(), model_lingsha.model_parts.end(), ModelPart::Body));
        auto hair_material_index = std::distance(model_lingsha.model_parts.begin(), std::find(model_lingsha.model_parts.begin(), model_lingsha.model_parts.end(), ModelPart::Hair));
        auto face_material_index = std::distance(model_lingsha.model_parts.begin(), std::find(model_lingsha.model_parts.begin(), model_lingsha.model_parts.end(), ModelPart::Face));

        PipelineVertexInputState vertex_input_state = std::move(PmxVertex::get_pipeline_vertex_input_state(0));
        star_rail_char_render_set.graphics_pipeline_body = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                vert_shader,
                frag_shader,
                vertex_input_state,
                {star_rail_char_render_set.descriptor_set->descriptor_set_layout(),
                 model_lingsha.sub_mesh_materials[body_material_index].descriptor_set->descriptor_set_layout()},
                {},
                true,
                false,
                m_graphics.settings().msaa,
                SpecializationConstants({{0, bytes(ModelPart::Body)}})});
        star_rail_char_render_set.graphics_pipeline_hair = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                vert_shader,
                frag_shader,
                vertex_input_state,
                {star_rail_char_render_set.descriptor_set->descriptor_set_layout(),
                 model_lingsha.sub_mesh_materials[hair_material_index].descriptor_set->descriptor_set_layout()},
                {},
                true,
                false,
                m_graphics.settings().msaa,
                SpecializationConstants({{0, bytes(ModelPart::Hair)}})});
        star_rail_char_render_set.graphics_pipeline_face = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                vert_shader,
                frag_shader,
                vertex_input_state,
                {star_rail_char_render_set.descriptor_set->descriptor_set_layout(),
                 model_lingsha.sub_mesh_materials[face_material_index].descriptor_set->descriptor_set_layout()},
                {},
                true,
                false,
                m_graphics.settings().msaa,
                SpecializationConstants({{0, bytes(ModelPart::Face)}})});
        star_rail_char_render_set.main_light.set_direction(glm::vec3(1.0f, -1.0f, -1.0f));

        star_rail_char_render_set_renderer.func_get_viewport = viewport::get_full_viewport;
        star_rail_char_render_set_renderer.func_get_scissor = scissor::get_full_scissor;
        star_rail_char_render_set_renderer.render_set = &star_rail_char_render_set;
    }

    JRenderer::~JRenderer()
    {
    }

    void JRenderer::set_msaa(const vk::SampleCountFlagBits &msaa)
    {
        m_graphics.set_msaa(msaa);

        star_rail_char_render_set.graphics_pipeline_body = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                std::make_shared<VertexShader>(m_graphics.logical_device(), "res/shaders/star_rail_vert.spv"),
                std::make_shared<FragmentShader>(m_graphics.logical_device(), "res/shaders/star_rail_frag.spv"),
                PmxVertex::get_pipeline_vertex_input_state(0),
                {star_rail_char_render_set.descriptor_set->descriptor_set_layout(),
                 model_lingsha.sub_mesh_materials[0].descriptor_set->descriptor_set_layout()},
                {},
                true,
                false,
                msaa});

        m_imgui_context.on_msaa_changed();
    }

    void JRenderer::new_imgui_frame()
    {
        m_imgui_context.new_frame();
    }

    void JRenderer::new_frame(const TickContext &context)
    {
        {
            ZoneScoped;
            tick(context);
        }
        {
            ZoneScoped;
            m_imgui_context.pre_draw();
            m_graphics.draw(
                std::vector<IRenderSetRenderer *>{&star_rail_char_render_set_renderer, &m_imgui_context});
        }
    }

    void JRenderer::tick(const TickContext &context)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // model_lingsha.model_matrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        camera_controller.tick(context);

        float view_matrix[16];
        camera_view_matrix(&camera, view_matrix);
        star_rail_char_render_set.view_matrix = glm::make_mat4(view_matrix);
    }

} // namespace jre
