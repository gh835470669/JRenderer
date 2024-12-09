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
#include "jrenderer/concrete_uniform_buffers.h"
#include "tracy/Tracy.hpp"
#include <ranges>

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
        Graphics m_graphics;
        imgui::ImguiContext m_imgui_context;

        void tick(const TickContext &context) override;

    public:
        PmxModel model_lingsha;
        StarRailCharRenderSet star_rail_char_render_set;
        StarRailCharRenderSetRenderer star_rail_char_render_set_renderer;

        template <typename TextureType = Texture2D>
        static PmxModel load_lingsha(const Graphics &graphics, const CommandBuffer &command_buffer, vk::DescriptorBufferInfo light_info, bool use_mipmaps = true)
        {
            ZoneScoped;
            PmxModel model;
            PmxFile pmx_file("res/model/HonkaiStarRail/lingsha/lingsha.pmx");
            PmxMeshData pmx_mesh_data = convert_to<PmxMeshData>(pmx_file);
            std::array model_parts = {
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Face,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Hair,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Body,
                ModelPart::Face,
                ModelPart::Face};

            auto filter_duplicate_sub_meshes = std::views::iota(0u, static_cast<uint32_t>(pmx_mesh_data.sub_mesh_indices.size())) |
                                               std::views::filter([](int i)
                                                                  { return i != 1 && i != 10 && i != 13; }); // 过滤掉重复的网格，不然会闪
            std::vector<uint32_t> filtered_sub_mesh_indexes(filter_duplicate_sub_meshes.begin(), filter_duplicate_sub_meshes.end());
            std::ranges::sort(filtered_sub_mesh_indexes, [&model_parts](uint32_t a, uint32_t b)
                              { return model_parts[a] < model_parts[b]; }); // 按部位排序，相同的部位相同的材质，减少bind pipeline的次数

            auto remove_duplicate_sub_meshes = filtered_sub_mesh_indexes |
                                               std::views::transform([&pmx_mesh_data](const auto &sub_mesh_index)
                                                                     { return pmx_mesh_data.sub_mesh_indices[sub_mesh_index]; });
            pmx_mesh_data.sub_mesh_indices = std::move(std::vector<PmxMeshData::SubMeshRegion>(remove_duplicate_sub_meshes.begin(), remove_duplicate_sub_meshes.end()));
            model.mesh = std::make_shared<PmxMesh>(graphics.logical_device(), command_buffer, pmx_mesh_data);

            auto texture_names = convert_to<std::vector<PmxTextureName>>(pmx_file);
            std::vector<std::shared_ptr<TextureType>> textures;
            STBTextureLoader texture_loader{};
            auto texture_light_map = std::make_shared<TextureType>(graphics.logical_device(),
                                                                   command_buffer,
                                                                   Texture2DCreateInfo{
                                                                       .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_LightMap_L.png").get(),
                                                                       .use_mipmaps = use_mipmaps});
            auto texture_cool_ramp = std::make_shared<TextureType>(graphics.logical_device(),
                                                                   command_buffer,
                                                                   Texture2DCreateInfo{
                                                                       .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Cool_Ramp.png").get(),
                                                                       .use_mipmaps = use_mipmaps,
                                                                       .address_mode = vk::SamplerAddressMode::eClampToEdge});
            auto texture_warm_ramp = std::make_shared<TextureType>(graphics.logical_device(),
                                                                   command_buffer,
                                                                   Texture2DCreateInfo{
                                                                       .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Warm_Ramp.png").get(),
                                                                       .use_mipmaps = use_mipmaps,
                                                                       .address_mode = vk::SamplerAddressMode::eClampToEdge});
            auto texture_light_map_hair = std::make_shared<TextureType>(graphics.logical_device(),
                                                                        command_buffer,
                                                                        Texture2DCreateInfo{
                                                                            .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_LightMap.png").get(),
                                                                            .use_mipmaps = use_mipmaps});
            auto texture_cool_ramp_hair = std::make_shared<TextureType>(graphics.logical_device(),
                                                                        command_buffer,
                                                                        Texture2DCreateInfo{
                                                                            .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_Cool_Ramp.png").get(),
                                                                            .use_mipmaps = use_mipmaps,
                                                                            .address_mode = vk::SamplerAddressMode::eClampToEdge});
            auto texture_warm_ramp_hair = std::make_shared<TextureType>(graphics.logical_device(),
                                                                        command_buffer,
                                                                        Texture2DCreateInfo{
                                                                            .image_view = texture_loader.load("res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_Warm_Ramp.png").get(),
                                                                            .use_mipmaps = use_mipmaps,
                                                                            .address_mode = vk::SamplerAddressMode::eClampToEdge});

            for (const auto &texture_name : texture_names)
            {
                auto texture_image_data = texture_loader.load(std::filesystem::path("res/model/HonkaiStarRail/lingsha").append(texture_name).string());
                auto texture = std::make_shared<TextureType>(graphics.logical_device(), command_buffer, Texture2DCreateInfo{.image_view = texture_image_data.get(), .use_mipmaps = use_mipmaps});
                textures.push_back(texture);
            }

            auto remove_duplicate_model_part = filtered_sub_mesh_indexes | std::views::transform([&model_parts](const auto &sub_mesh_index)
                                                                                                 { return model_parts[sub_mesh_index]; });
            model.model_parts = std::move(std::vector<ModelPart>(remove_duplicate_model_part.begin(), remove_duplicate_model_part.end()));

            model.sub_mesh_materials.reserve(pmx_file.model().material_count);
            for (size_t i : filtered_sub_mesh_indexes)
            {
                const pmx::PmxMaterial &pmx_material = pmx_file.model().materials[i];
                if (model_parts[i] == ModelPart::Hair)
                {
                    auto &material = model.sub_mesh_materials.emplace_back(
                        std::move(std::vector<std::shared_ptr<Texture2D>>{
                            textures[pmx_material.diffuse_texture_index],
                            texture_light_map_hair,
                            texture_cool_ramp_hair,
                            texture_warm_ramp_hair}),
                        std::move(graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                  {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})),
                        std::make_unique<UniformBuffer<PmxUniformPerObject>>(graphics.logical_device()));
                    material.descriptor_set->update_descriptor_sets(
                        {material.uniform_buffer->descriptor(),
                         material.textures[0]->descriptor(),
                         material.textures[1]->descriptor(),
                         material.textures[2]->descriptor(),
                         material.textures[3]->descriptor()});
                }
                else if (model_parts[i] == ModelPart::Face)
                {
                    auto &material = model.sub_mesh_materials.emplace_back(
                        std::move(std::vector<std::shared_ptr<Texture2D>>{
                            textures[pmx_material.diffuse_texture_index]}),
                        std::move(graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                  {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})),
                        std::make_unique<UniformBuffer<PmxUniformPerObject>>(graphics.logical_device()));
                    material.descriptor_set->update_descriptor_sets(
                        {material.uniform_buffer->descriptor(),
                         material.textures[0]->descriptor(),
                         texture_light_map->descriptor(),
                         texture_cool_ramp->descriptor(),
                         texture_warm_ramp->descriptor()});
                }
                else if (model_parts[i] == ModelPart::Body)
                {
                    auto &material = model.sub_mesh_materials.emplace_back(
                        std::move(std::vector<std::shared_ptr<Texture2D>>{
                            textures[pmx_material.diffuse_texture_index],
                            texture_light_map,
                            texture_cool_ramp,
                            texture_warm_ramp}),
                        std::move(graphics.create_descriptor_set({{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                                                  {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                                                  {4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})),
                        std::make_unique<UniformBuffer<PmxUniformPerObject>>(graphics.logical_device()));
                    material.descriptor_set->update_descriptor_sets(
                        {material.uniform_buffer->descriptor(),
                         material.textures[0]->descriptor(),
                         material.textures[1]->descriptor(),
                         material.textures[2]->descriptor(),
                         material.textures[3]->descriptor()});
                }
            }
            return model;
        }

        InputManager input_manager;

        Camera camera;
        CameraController camera_controller;
    };

}
