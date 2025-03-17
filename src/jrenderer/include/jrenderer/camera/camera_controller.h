#pragma once

#include "jrenderer/window.h"
#include "jrenderer/camera/camera.h"
#include "jrenderer/input_manager.h"
#include "jrenderer/ticker/scene_ticker.h"
#include <memory>

namespace jre
{
    class CameraController : public ISceneTicker
    {
    public:
        CameraController(InputManager &input_manager);

        Camera default_camera;

        void tick(SceneTickContext context) override;

    private:
        InputMap m_input_map;
    };
}