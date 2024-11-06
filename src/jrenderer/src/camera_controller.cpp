#define CAMERA_IMPLEMENTATION
#include "jrenderer/camera/camera_controller.h"
#include <fmt/core.h>
#include <fmt/ranges.h>

enum class ButtonCameraControl {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,

    RotateMode,
    RotateYaw,
    RotatePitch,
};

jre::CameraController::CameraController(InputManager &input_manager) : m_input_map(input_manager)
{
    m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveForward), input_manager.keyboard_device(), gainput::KeyW);
    m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveBackward), input_manager.keyboard_device(), gainput::KeyS);
    m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveLeft), input_manager.keyboard_device(), gainput::KeyA);
    m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::MoveRight), input_manager.keyboard_device(), gainput::KeyD);

    m_input_map.MapBool(static_cast<UserButtonId>(ButtonCameraControl::RotateMode), input_manager.mouse_device(), gainput::MouseButtonRight);
    m_input_map.MapFloat(static_cast<UserButtonId>(ButtonCameraControl::RotateYaw), input_manager.mouse_device(), gainput::MouseAxisX);
    m_input_map.MapFloat(static_cast<UserButtonId>(ButtonCameraControl::RotatePitch), input_manager.mouse_device(), gainput::MouseAxisY);
}

void jre::CameraController::tick(const TickContext &context)
{
    if (!camera)
        return;

    static float velocity = 1.0f; // 1 per second
    float distance = velocity * context.delta_time;
    if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveForward)))
    {
        camera_move(camera, {distance, 0, 0});
    }
    if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveBackward)))
    {
        camera_move(camera, {-distance, 0, 0});
    }
    if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveLeft)))
    {
        camera_move(camera, {0, 0, -distance});
    }
    if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::MoveRight)))
    {
        camera_move(camera, {0, 0, distance});
    }

    if (m_input_map.GetBool(static_cast<UserButtonId>(ButtonCameraControl::RotateMode)))
    {
        // 左上角0, 0 右下角1,1
        camera_rotate(camera, {m_input_map.GetFloatDelta(static_cast<UserButtonId>(ButtonCameraControl::RotatePitch)) * context.delta_time * 1000,
                               m_input_map.GetFloatDelta(static_cast<UserButtonId>(ButtonCameraControl::RotateYaw)) * context.delta_time * 1000,
                               0});
    }
}