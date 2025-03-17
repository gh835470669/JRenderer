#include "jrenderer/ticker/scene_ticker.h"
#include "jrenderer/drawer/scene_drawer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tracy/Tracy.hpp"

namespace jre
{
    void SceneUBOTicker::tick(SceneTickContext context)
    {
        ZoneScoped;
        Scene &scene = context.scene;
        UniformScene ubo_scene = scene.scene_buffers[context.cur_frame];
        ubo_scene.main_light = convert_to<UniformLight>(scene.main_light);
        auto &main_view = scene.render_viewports[0];
        camera_view_matrix(&main_view.camera, glm::value_ptr(ubo_scene.camera_trans.view));
        ubo_scene.camera_trans.proj = main_view.projection;
        ubo_scene.camera_trans.view_proj = ubo_scene.camera_trans.proj * ubo_scene.camera_trans.view;
        scene.scene_buffers[context.cur_frame] = ubo_scene;

        for (auto &model : scene.models)
        {
            UniformPerObject ubo_obj = model.transform.ubo(context.cur_frame);
            ubo_obj.mvp.model_view = ubo_scene.camera_trans.view * ubo_obj.mvp.model;
            ubo_obj.mvp.model_view_proj = ubo_scene.camera_trans.proj * ubo_obj.mvp.model_view;
            model.transform.set_ubo(ubo_obj, context.cur_frame);

            for (auto &material_instance : model.materials)
            {
                material_instance->update_descriptor_set_data(context.cur_frame);
            }
        }
    }
}