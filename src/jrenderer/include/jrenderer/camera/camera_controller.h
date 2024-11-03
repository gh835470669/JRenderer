#pragma once

#include "jrenderer/window.h"
#include "jrenderer/camera/camera.h"
#include "jrenderer/input_manager.h"
#include "details/main_loop_context.h"
#include <memory>

namespace jre
{
    class CameraController : ITickable
    {
    public:
        CameraController(InputManager &input_manager);

        Camera *camera;

        void tick(const TickContext &context) override;

    private:
        InputMap m_input_map;

        float m_rotate_start_pos_x = 0.0f;
        float m_rotate_start_pos_y = 0.0f;
    };
}