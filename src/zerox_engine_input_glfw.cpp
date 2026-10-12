#include <GLFW/glfw3.h>

#include "zerox_engine_input.h"

namespace
{
    void handleJoystickEvent(int id, int event)
    {
        if (id < GLFW_JOYSTICK_1 || id > GLFW_JOYSTICK_LAST)
        {
            return;
        }

        const uint16_t mask = 1 << id;

        switch (event)
        {
        case GLFW_CONNECTED:
            ZeroX::globalControllerState |= mask;
            break;
        case GLFW_DISCONNECTED:
            ZeroX::globalControllerState &= ~mask;
            break;
        default:
            break;
        }
    }
}

namespace ZeroX
{
    void monitorJoystickConnections()
    {
        glfwSetJoystickCallback(::handleJoystickEvent);
    }

    void updateGamePad(GamePad* gamePad, int id)
    {
        
    }

    void updateInput(Input* input)
    {
        
    }
}