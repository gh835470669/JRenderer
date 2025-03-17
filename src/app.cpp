#include "app.h"
#include "tracy/Tracy.hpp"
#include "jrenderer/asset/model_lingsha.h"
#include "jrenderer/camera/camera_controller.h"

App::App(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
    : window(hinst, hprev, cmdline, show),
      renderer(this->window),
      imwin_debug(statistics, renderer)
{

    // AllocConsole();
    // freopen("CONOUT$", "w", stdout);
    // FreeConsole();
    jre::Graphics &graphics = renderer.graphics();
    jre::SceneDrawer &scene_drawer = renderer.scene_drawer();
    jre::Scene &scene = scene_drawer.scene;
    jre::Model &model = scene.models.emplace_back(load_lingsha(scene_drawer,
                                                               graphics.cpu_frames().size(),
                                                               graphics.logical_device(),
                                                               graphics.physical_device(),
                                                               graphics.transfer_queue(),
                                                               vk::shared::allocate_one_command_buffer(graphics.transfer_command_pool())));
    model.transform.set_model(glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    auto camera_controller = std::make_shared<jre::CameraController>(renderer.input_manager);
    camera_controller->default_camera = camera_init();
    camera_controller->default_camera.target_position = {0.0f, 15.0f, 40.0f};
    auto orient = glm::quat(glm::vec3(0, 0, 0));
    camera_controller->default_camera.orientation = {orient.x, orient.y, orient.z, orient.w};
    renderer.scene_ticker().tickers.emplace_back(camera_controller);

    jre::RenderViewport &viewport = scene.render_viewports.front();
    viewport.camera = camera_controller->default_camera;
    viewport.projection = glm::perspective(glm::radians(45.0f), viewport.viewport.width / viewport.viewport.height, 0.1f, 1000.f);
    viewport.projection[1][1] *= -1;

    scene.main_light.set_direction(glm::vec3(1.0f, -1.0f, -1.0f));
}

int App::main_loop()
{
    bool exit = false;
    while (!exit)
    {
        ZoneScoped;
        renderer.input_manager.input_manager().Update();

        MSG msg = window.ProcessMessage(std::bind(&gainput::InputManager::HandleMessage, &renderer.input_manager.input_manager(), std::placeholders::_1));
        if (msg.message == WM_QUIT)
        {
            exit = true;
        }

        statistics.tick();
        {
            jre::imgui::ScopedFrame scoped_imgui_frame;
            imwin_debug.tick();
        }
        jre::TickContext tick_context;
        tick_context.delta_time = statistics.frame_counter_graph.frame_counter.mspf() / 1000.f;
        renderer.new_frame(tick_context);

        FrameMark;
    }
    renderer.graphics().wait_idle(); // wait for the device to be idle before shutting down
    return exit;
}
