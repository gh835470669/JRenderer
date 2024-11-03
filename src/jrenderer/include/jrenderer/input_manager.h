#pragma once

#include <gainput/gainput.h>

namespace jre
{
    using InputMap = gainput::InputMap;
    using UserButtonId = gainput::UserButtonId;
    using DeviceId = gainput::DeviceId;

    class InputManager
    {
    public:
        InputManager();

        gainput::InputManager &input_manager() { return m_input_manager; }
        operator gainput::InputManager &() { return m_input_manager; }
        DeviceId mouse_device() const { return m_mouse_device; }
        DeviceId keyboard_device() const { return m_keyboard_device; }

    private:
        gainput::InputManager m_input_manager;
        gainput::DeviceId m_mouse_device;
        gainput::DeviceId m_keyboard_device;
    };
}