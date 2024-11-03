#include "jrenderer/input_manager.h"

namespace jre
{
    InputManager::InputManager() : m_input_manager(),
                                   m_mouse_device(m_input_manager.CreateDevice<gainput::InputDeviceMouse>()),
                                   m_keyboard_device(m_input_manager.CreateDevice<gainput::InputDeviceKeyboard>()) {}
}