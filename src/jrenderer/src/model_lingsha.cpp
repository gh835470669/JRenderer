#pragma once

#include "jrenderer/asset/model_lingsha.h"
#include "jrenderer/asset/pmx_file.h"
#include "jrenderer/asset/star_rail_material.h"
#include <ranges>
#include <unordered_set>

namespace jre
{
    Model load_lingsha(SceneDrawer &scene_drawer,
                       uint32_t frame_count,
                       vk::SharedDevice device,
                       vk::PhysicalDevice physical_device,
                       vk::SharedQueue transfer_queue,
                       vk::SharedCommandBuffer command_buffer)
    {
        Model model = scene_drawer.factory.create();
        PmxFile pmx_file("res/model/HonkaiStarRail/lingsha/lingsha.pmx");
        PmxMeshBuilder<jre::Vertex, uint32_t> builder(device,
                                                      physical_device,
                                                      command_buffer.get(),
                                                      transfer_queue.get(),
                                                      pmx_file);
        std::shared_ptr<Mesh> mesh = builder.build_shared();
        std::unordered_set<int> removed_indices = {1, 10, 13};
        std::vector<uint32_t> filtered_sub_mesh_indexes = std::views::iota(0u, static_cast<uint32_t>(mesh->sub_meshes.size())) |
                                                          std::views::filter([](int i)
                                                                             { return i != 1 && i != 10 && i != 13; }) |
                                                          std::ranges::to<std::vector>(); // 过滤掉重复的网格，不然会闪
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

        std::ranges::sort(filtered_sub_mesh_indexes, [&model_parts](uint32_t a, uint32_t b)
                          { return model_parts[a] < model_parts[b]; }); // 按部位排序，相同的部位相同的材质，减少bind pipeline的次数

        mesh->sub_meshes = filtered_sub_mesh_indexes |
                           std::views::transform([&mesh](uint32_t sub_mesh_index)
                                                 { return mesh->sub_meshes[sub_mesh_index]; }) |
                           std::ranges::to<std::vector>();
        model.mesh = mesh;

        std::unordered_map<std::string, vk::SharedShaderModule> shader_cache;
        StarRailMaterialBuilder material_builder(
            scene_drawer.render_pipelines,
            scene_drawer.pipeline_layout_builder,
            scene_drawer.pipeline_builder,
            device,
            physical_device,
            command_buffer.get(),
            transfer_queue.get());
        material_builder.builder.shader_cache = &shader_cache;
        material_builder.builder.vertex_shader_info.constants.set_constant(0, bytes(ModelPart::Body));
        material_builder.builder.fragment_shader_info.constants.set_constant(0, bytes(ModelPart::Body));
        Material body_material = material_builder.build();
        material_builder.builder.vertex_shader_info.constants.set_constant(0, bytes(ModelPart::Hair));
        material_builder.builder.fragment_shader_info.constants.set_constant(0, bytes(ModelPart::Hair));
        Material hair_material = material_builder.build();
        material_builder.builder.vertex_shader_info.constants.set_constant(0, bytes(ModelPart::Face));
        material_builder.builder.fragment_shader_info.constants.set_constant(0, bytes(ModelPart::Face));
        Material face_material = material_builder.build();

        StarRailOutlineMaterialBuilder outline_material_builder(
            scene_drawer.render_pipelines,
            scene_drawer.pipeline_layout_builder,
            scene_drawer.pipeline_builder,
            device,
            physical_device,
            command_buffer.get(),
            transfer_queue.get());
        outline_material_builder.builder.shader_cache = &shader_cache;
        outline_material_builder.builder.vertex_shader_info.constants.set_constant(0, bytes(ModelPart::Body));
        outline_material_builder.builder.fragment_shader_info.constants.set_constant(0, bytes(ModelPart::Body));
        Material body_outline_material = outline_material_builder.build();
        outline_material_builder.builder.vertex_shader_info.constants.set_constant(0, bytes(ModelPart::Face));
        outline_material_builder.builder.fragment_shader_info.constants.set_constant(0, bytes(ModelPart::Face));
        Material face_outline_material = outline_material_builder.build();

        std::unordered_map<std::string, Texture> texture_cache;
        StarRailMaterialInstanceBuilder body_material_instance_builder{
            device,
            physical_device,
            command_buffer.get(),
            transfer_queue.get(),
            frame_count,
            body_material,
            &texture_cache,
            {},
            {},
            {},
            {}};
        body_material_instance_builder.filename_light_map = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_LightMap_L.png";
        body_material_instance_builder.filename_cool_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Cool_Ramp.png";
        body_material_instance_builder.filename_warm_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Warm_Ramp.png";
        StarRailMaterialInstanceBuilder hair_material_instance_builder{
            device,
            physical_device,
            command_buffer.get(),
            transfer_queue.get(),
            frame_count,
            hair_material,
            &texture_cache,
            {},
            {},
            {},
            {}};
        hair_material_instance_builder.filename_light_map = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_LightMap.png";
        hair_material_instance_builder.filename_cool_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_Cool_Ramp.png";
        hair_material_instance_builder.filename_warm_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Hair_Warm_Ramp.png";
        StarRailMaterialInstanceBuilder face_material_instance_builder{
            device,
            physical_device,
            command_buffer.get(),
            transfer_queue.get(),
            frame_count,
            face_material,
            &texture_cache,
            {},
            {},
            {},
            {}};
        face_material_instance_builder.filename_light_map = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_LightMap_L.png";
        face_material_instance_builder.filename_cool_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Cool_Ramp.png";
        face_material_instance_builder.filename_warm_ramp = "res/model/HonkaiStarRail/lingsha/Avatar_Lingsha_00_Body_Warm_Ramp.png";
        auto get_material = [&](ModelPart part) -> StarRailMaterialInstanceBuilder &
        {
            switch (part)
            {
            case ModelPart::Body:
                return body_material_instance_builder;
            case ModelPart::Hair:
                return hair_material_instance_builder;
            case ModelPart::Face:
                return face_material_instance_builder;
            default:
                throw std::runtime_error("Unknown part");
            }
        };

        StarRailOutlineMaterialInstanceBuilder body_outline_material_instance_builder{device,
                                                                                      physical_device,
                                                                                      command_buffer.get(),
                                                                                      transfer_queue.get(),
                                                                                      frame_count,
                                                                                      body_outline_material,
                                                                                      &texture_cache,
                                                                                      {}};
        StarRailOutlineMaterialInstanceBuilder face_outline_material_instance_builder{device,
                                                                                      physical_device,
                                                                                      command_buffer.get(),
                                                                                      transfer_queue.get(),
                                                                                      frame_count,
                                                                                      face_outline_material,
                                                                                      &texture_cache,
                                                                                      {}};
        auto get_outline_material = [&](ModelPart part) -> StarRailOutlineMaterialInstanceBuilder &
        {
            switch (part)
            {
            case ModelPart::Body:
            case ModelPart::Hair:
                return body_outline_material_instance_builder;
            case ModelPart::Face:
                return face_outline_material_instance_builder;
            default:
                throw std::runtime_error("Unknown part");
            }
        };

        auto build_base_instance = [&](ModelPart part, const std::wstring &diffuse_filename)
        {
            StarRailMaterialInstanceBuilder &material_instance_builder = get_material(part);
            material_instance_builder.filename_diffuse = std::filesystem::path("res/model/HonkaiStarRail/lingsha").append(diffuse_filename).string();
            return std::static_pointer_cast<IMaterialInstance>(material_instance_builder.build_shared());
        };

        auto build_outline_instance = [&](ModelPart part, const std::wstring &diffuse_filename)
        {
            StarRailOutlineMaterialInstanceBuilder &material_instance_builder = get_outline_material(part);
            material_instance_builder.filename_diffuse = std::filesystem::path("res/model/HonkaiStarRail/lingsha").append(diffuse_filename).string();
            return std::static_pointer_cast<IMaterialInstance>(material_instance_builder.build_shared());
        };

        auto get_material_instance = [&pmx_file, &get_material, &model_parts](uint32_t i, std::unordered_map<std::wstring, std::shared_ptr<IMaterialInstance>> &cache, std::function<std::shared_ptr<IMaterialInstance>(ModelPart, const std::wstring &)> build_material_instance)
        {
            std::wstring diffuse_filename = pmx_file.model().textures[pmx_file.model().materials[i].diffuse_texture_index];
            if (cache.contains(diffuse_filename))
            {
                return cache[diffuse_filename];
            }
            ModelPart part = model_parts[i];
            auto material_instance = build_material_instance(part, diffuse_filename);
            cache[diffuse_filename] = material_instance;
            return std::static_pointer_cast<IMaterialInstance>(material_instance);
        };

        std::unordered_map<std::wstring, std::shared_ptr<IMaterialInstance>> base_materials_cache;
        std::unordered_map<std::wstring, std::shared_ptr<IMaterialInstance>> outline_materials_cache;
        model.materials = filtered_sub_mesh_indexes |
                          std::views::transform(
                              std::bind(get_material_instance, std::placeholders::_1, base_materials_cache, build_base_instance)) |
                          std::ranges::to<std::vector>();
        std::ranges::transform(filtered_sub_mesh_indexes,
                               std::back_inserter(model.materials),
                               std::bind(get_material_instance, std::placeholders::_1, outline_materials_cache, build_outline_instance));

        return std::move(model);
    }
}