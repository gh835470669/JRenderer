#define CAMERA_IMPLEMENTATION
#include "jrenderer/camera/camera_controller.h"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <glm/gtx/quaternion.hpp>

namespace jre
{
    enum class ButtonCameraControl
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,

        UpDownMode,
        MoveUp,
        MoveDown,

        RotateMode,
        RotateYaw,
        RotatePitch,

        Reset,
    };

    CameraController::CameraController(InputManager &input_manager) : m_input_map(input_manager)
    {
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveForward), input_manager.keyboard_device(), gainput::KeyW);
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveBackward), input_manager.keyboard_device(), gainput::KeyS);
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveLeft), input_manager.keyboard_device(), gainput::KeyA);
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveRight), input_manager.keyboard_device(), gainput::KeyD);

        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::RotateMode), input_manager.mouse_device(), gainput::MouseButtonRight);
        m_input_map.MapFloat(static_cast<UserButtonId>(ButtonCameraControl::RotateYaw), input_manager.mouse_device(), gainput::MouseAxisX);
        m_input_map.MapFloat(static_cast<UserButtonId>(ButtonCameraControl::RotatePitch), input_manager.mouse_device(), gainput::MouseAxisY);

        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::UpDownMode), input_manager.keyboard_device(), gainput::KeyCtrlL);
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveUp), input_manager.keyboard_device(), gainput::KeyW);
        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveDown), input_manager.keyboard_device(), gainput::KeyS);
        // 如果map 能表达与或关系就好了， MoveUp 是gainput::KeyAltL & gainput::KeyW

        m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::Reset), input_manager.keyboard_device(), gainput::KeyR);
    }

    void CameraController::tick(const TickContext &context)
    {
        if (!camera)
            return;

        if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::Reset)))
        {
            reset_default_camera();
        }

        static float velocity = 10.0f; // 1 per second
        float distance = velocity * context.delta_time;

        if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveLeft)))
        {
            camera_move(camera, {0, 0, -distance});
        }
        if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveRight)))
        {
            camera_move(camera, {0, 0, distance});
        }

        if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::UpDownMode)))
        {
            if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveUp)))
            {
                camera_move(camera, {0, distance, 0});
            }
            if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveDown)))
            {
                camera_move(camera, {0, -distance, 0});
            }
        }
        else
        {
            if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveForward)))
            {
                camera_move(camera, {distance, 0, 0});
            }
            if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveBackward)))
            {
                camera_move(camera, {-distance, 0, 0});
            }
        }

        if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::RotateMode)))
        {
            // 左上角0, 0 右下角1,1
            camera_rotate(camera, {-m_input_map.GetFloatDelta(static_cast<UserButtonId>(ButtonCameraControl::RotatePitch)) * context.delta_time * 1000,
                                   -m_input_map.GetFloatDelta(static_cast<UserButtonId>(ButtonCameraControl::RotateYaw)) * context.delta_time * 1000,
                                   0});
        }
    }

    void CameraController::reset_default_camera()
    {
        if (!camera)
            return;

        *camera = default_camera;
    }
}