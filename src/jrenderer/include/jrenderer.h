#pragma once

#include "details/imgui_context.h"
#include "jrenderer/graphics.h"
#include "jrenderer/window.h"
#include "jrenderer/render_set.h"
#include "jrenderer/mesh.h"
#include "jrenderer/model.h"
#include "jrenderer/camera/camera_controller.h"
#include "jrenderer/input_manager.h"
#include "jrenderer/star_rail_char_render_set.h"
#include "tracy/Tracy.hpp"

namespace jre
{
    class JRenderer : public ITickable
    {
        friend class JRendererRebuilder;

    public:
        JRenderer(Window &window);
        ~JRenderer();

        inline Graphics &graphics() { return m_graphics; }

        void set_msaa(const vk::SampleCountFlagBits &msaa);

        void new_imgui_frame();
        void new_frame(const TickContext &context);

    private:
        Window &m_window;
        // VulkanPipelineBuilder m_pipeline_builder;
        // std::unique_ptr<VulkanPipeline> m_pipeline;

        Graphics m_graphics;

        imgui::ImguiContext m_imgui_context;

        void tick(const TickContext &context) override;

    public:
        PmxModel model_lingsha;
        StarRailCharRenderSet star_rail_char_render_set;
        StarRailCharRenderSetRenderer star_rail_char_render_set_renderer;

        template <typename TextureType = Texture2D>
        static PmxModel load_lingsha(const Graphics &graphics, const CommandBuffer &command_buffer, bool use_mipmaps = true)
        {
            ZoneScoped;
            PmxModel model;
            PmxFile pmx_file("res/model/HonkaiStarRail/lingsha/lingsha.pmx");
            PmxMeshData pmx_mesh_data = convert_to<PmxMeshData>(pmx_file);
            model.mesh = std::make_shared<PmxMesh>(graphics.logical_device(), command_buffer, pmx_mesh_data);
            auto texture_names = convert_to<std::vector<PmxTextureName>>(pmx_file);
            std::vector<std::shared_ptr<TextureType>> textures;
            auto texture_loader = STBTextureLoader();
            for (const auto &texture_name : texture_names)
            {
                auto texture_image_data = texture_loader.load(std::filesystem::path("res/model/HonkaiStarRail/lingsha").append(texture_name).string());
                auto texture = std::make_shared<TextureType>(graphics.logical_device(), command_buffer, Texture2DCreateInfo{.image_view = texture_image_data.get(), .use_mipmaps = use_mipmaps});
                textures.push_back(texture);
            }

            model.sub_mesh_materials.reserve(pmx_file.model().material_count);
            for (size_t i = 0; i < pmx_file.model().material_count; ++i)
            {
                const pmx::PmxMaterial &pmx_material = pmx_file.model().materials[i];
                model.sub_mesh_materials.emplace_back(
                    std::vector<std::shared_ptr<Texture2D>>{
                        textures[pmx_material.diffuse_texture_index],
                    },
                    std::move(graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                              {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})),
                    std::make_unique<UniformBuffer<UniformBufferObject>>(graphics.logical_device()));

                auto &material = model.sub_mesh_materials[i];
                std::array<const UniformBuffer<UniformBufferObject> *, 1> uniform_buffers{material.uniform_buffer.get()};
                material.descriptor_set->update_descriptor_sets(
                    uniform_buffers.begin(),
                    material.textures.begin());
            }
            return model;
        }

        InputManager input_manager;

        Camera camera;
        CameraController camera_controller;
    };

}
