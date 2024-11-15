#include "jrenderer.h"
#include <iostream>
#include "tracy/Tracy.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <chrono>
#include "jrenderer/asset/obj_file.h"

namespace jre
{
    JRenderer::JRenderer(Window &window) : m_window(window),
                                           m_graphics(&window),
                                           m_res_meshes(m_graphics.logical_device(), std::move(m_graphics.command_pool()->allocate_command_buffer())),
                                           m_res_textures(m_graphics.logical_device(), std::move(m_graphics.command_pool()->allocate_command_buffer())),
                                           model(
                                               m_res_meshes.get_mesh("res/model/viking_room/viking_room.obj", ObjLoader<>()),
                                               m_res_textures.get_texture("res/model/viking_room/viking_room.png"),
                                               m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                                 {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}),
                                               std::make_unique<UniformBuffer<UniformBufferObject>>(m_graphics.logical_device())),
                                           model2(
                                               m_res_meshes.get_mesh("res/model/viking_room/viking_room.obj", ObjLoader<>()),
                                               m_res_textures.get_texture("res/model/viking_room/viking_room.png"),
                                               m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                                 {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}),
                                               std::make_unique<UniformBuffer<UniformBufferObject>>(m_graphics.logical_device())),
                                           m_imgui_context(m_window, m_graphics),
                                           m_camera_controller(m_input_manager)
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
        model.descriptor_set->update_descriptor_sets<UniformBufferObject, const UniformBuffer<UniformBufferObject> *const, const Texture2D *const>(std::array{model.uniform_buffer.get()}, std::array{model.texture.get()});
        model2.descriptor_set->update_descriptor_sets<UniformBufferObject, const UniformBuffer<UniformBufferObject> *const, const Texture2D *const>(std::array{model2.uniform_buffer.get()}, std::array{model2.texture.get()});

        render_set_renderer.func_get_viewport = viewport::get_full_viewport;
        render_set_renderer.func_get_scissor = scissor::get_full_scissor;
        render_set_renderer.render_set = &render_set;

        m_input_manager.input_manager().SetDisplaySize(m_window.width(), m_window.height());

        m_camera_controller.default_camera = camera_init();
        m_camera_controller.default_camera.target_position = {0.0f, 15.0f, -40.0f};
        auto orient = glm::quat(glm::vec3(0, -glm::pi<float>(), 0));
        m_camera_controller.default_camera.orientation = {orient.x, orient.y, orient.z, orient.w};
        m_camera_controller.camera = &m_camera;
        m_camera_controller.reset_default_camera();

        PmxFile pmx_file("res/model/HonkaiStarRail/lingsha/lingsha.pmx");
        PmxMeshData pmx_mesh_data = convert_to<PmxMeshData>(pmx_file);
        model_lingsha.mesh = std::make_shared<PmxMesh>(m_graphics.logical_device(), *m_graphics.command_pool()->allocate_command_buffer(), pmx_mesh_data);
        auto texture_names = convert_to<std::vector<PmxTextureName>>(pmx_file);
        std::vector<std::shared_ptr<Texture2D>> textures;
        for (const auto &texture_name : texture_names)
        {
            textures.push_back(m_res_textures.get_texture(std::filesystem::path("res/model/HonkaiStarRail/lingsha").append(texture_name).string()));
        }

        model_lingsha.sub_mesh_materials.reserve(pmx_file.model().material_count);
        for (size_t i = 0; i < pmx_file.model().material_count; ++i)
        {
            const pmx::PmxMaterial &pmx_material = pmx_file.model().materials[i];
            model_lingsha.sub_mesh_materials.emplace_back(
                std::vector<std::shared_ptr<Texture2D>>{
                    textures[pmx_material.diffuse_texture_index],
                },
                std::move(m_graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                            {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})),
                std::make_unique<UniformBuffer<UniformBufferObject>>(m_graphics.logical_device()));

            auto &material = model_lingsha.sub_mesh_materials[i];
            material.descriptor_set->update_descriptor_sets<
                UniformBufferObject,
                const UniformBuffer<UniformBufferObject> *const,
                std::shared_ptr<Texture2D>>(
                std::array{material.uniform_buffer.get()},
                material.textures);
        }

        star_rail_char_render_set.graphics_pipeline = std::make_shared<GraphicsPipeline>(
            m_graphics.logical_device(),
            GraphicsPipelineCreateInfo{
                *m_graphics.render_pass(),
                std::make_shared<VertexShader>(m_graphics.logical_device(), "res/shaders/star_rail_vert.spv"),
                std::make_shared<FragmentShader>(m_graphics.logical_device(), "res/shaders/star_rail_frag.spv"),
                PmxVertex::get_pipeline_vertex_input_state(0),
                {model_lingsha.sub_mesh_materials[0].descriptor_set->descriptor_set_layout()},
                {},
                true,
                false});
        star_rail_char_render_set.render_objects.push_back(model_lingsha);

        star_rail_char_render_set_renderer.func_get_viewport = viewport::get_full_viewport;
        star_rail_char_render_set_renderer.func_get_scissor = scissor::get_full_scissor;
        star_rail_char_render_set_renderer.render_set = &star_rail_char_render_set;
    }

    JRenderer::~JRenderer()
    {
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
            m_graphics.draw(
                std::vector<IRenderSetRenderer *>{&star_rail_char_render_set_renderer, &m_imgui_context});
        }
    }

    void JRenderer::tick(const TickContext &context)
    {

        if (m_graphics.swap_chain()->extent().width == 0 || m_graphics.swap_chain()->extent().height == 0)
            return;
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        model.model_matrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model2.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        m_camera_controller.tick(context);

        float view_matrix[16];
        camera_view_matrix(&m_camera, view_matrix);
        render_set.view_matrix = glm::make_mat4(view_matrix);
        star_rail_char_render_set.view_matrix = render_set.view_matrix;
    }

} // namespace jre
