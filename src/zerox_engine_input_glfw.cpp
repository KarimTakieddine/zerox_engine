#include <GLFW/glfw3.h>

#include "zerox_engine_input.h"

namespace
{
    // TODO(Karim): Unordered map?

    int getGLFWButtonCode(ZeroX::Button button)
    {
        int result = -1;

        switch (button)
        {
        case ZeroX::Button::BUTTON_A:
            result = GLFW_GAMEPAD_BUTTON_A;
            break;
        case ZeroX::Button::BUTTON_B:
            result = GLFW_GAMEPAD_BUTTON_B;
            break;
        case ZeroX::Button::BUTTON_X:
            result = GLFW_GAMEPAD_BUTTON_X;
            break;
        case ZeroX::Button::BUTTON_Y:
            result = GLFW_GAMEPAD_BUTTON_Y;
            break;
        case ZeroX::Button::BUTTON_LB:
            result = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
            break;
        case ZeroX::Button::BUTTON_RB:
            result = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
            break;
        case ZeroX::Button::BUTTON_START:
            result = GLFW_GAMEPAD_BUTTON_START;
            break;
        case ZeroX::Button::BUTTON_BACK:
            result = GLFW_GAMEPAD_BUTTON_BACK;
            break;
        case ZeroX::Button::LEFT_CLICK:
            result = GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
            break;
        case ZeroX::Button::RIGHT_CLICK:
            result = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
            break;
        case ZeroX::Button::BUTTON_DPAD_UP:
            result = GLFW_GAMEPAD_BUTTON_DPAD_UP;
            break;
        case ZeroX::Button::BUTTON_DPAD_DOWN:
            result = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
            break;
        case ZeroX::Button::BUTTON_DPAD_LEFT:
            result = GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
            break;
        case ZeroX::Button::BUTTON_DPAD_RIGHT:
            result = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
            break;
        default:
            break;
        }

        return result;
    }

    void handleJoystickEvent(int id, int event)
    {
        if (id < GLFW_JOYSTICK_1 || id > GLFW_JOYSTICK_LAST)
        {
            return;
        }

        const uint16_t mask = 1 << static_cast<uint16_t>(id);

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

    void updateButtonState(uint64_t* packedState, const GLFWgamepadstate& state)
    {
        for (
            uint8_t button = static_cast<uint8_t>(ZeroX::Button::BUTTON_A);
            button < static_cast<uint8_t>(ZeroX::Button::BUTTON_LAST);
            ++button)
        {
            const int glfwButton = ::getGLFWButtonCode(static_cast<ZeroX::Button>(button));

            if (glfwButton < 0)
            {
                continue;
            }

            const unsigned char buttonState = state.buttons[static_cast<size_t>(glfwButton)];

            const uint64_t mask = 15ULL << ( button * 4 );

            const uint64_t currentState = ( *packedState ) & mask;
            ZeroX::KeyState nextState   = static_cast<ZeroX::KeyState>(currentState >> ( button * 4 ));

            switch (buttonState)
            {
            case GLFW_PRESS:
            {
                if ((nextState & ZeroX::KeyState::PRESSED) == ZeroX::KeyState::PRESSED)
                {
                    nextState &= ~ZeroX::KeyState::PRESSED_THIS_FRAME;
                }
                else
                {
                    nextState |= ZeroX::KeyState::PRESSED_THIS_FRAME;
                }

                nextState |= ZeroX::KeyState::PRESSED;
                nextState &= ~( ZeroX::KeyState::RELEASED | ZeroX::KeyState::RELEASED_THIS_FRAME );
                break;
            }
            case GLFW_RELEASE:
            {
                if ((nextState & ZeroX::KeyState::RELEASED) == ZeroX::KeyState::RELEASED)
                {
                    nextState &= ~ZeroX::KeyState::RELEASED_THIS_FRAME;
                }
                else
                {
                    nextState |= ZeroX::KeyState::RELEASED_THIS_FRAME;
                }

                nextState |= ZeroX::KeyState::RELEASED;
                nextState &= ~( ZeroX::KeyState::PRESSED | ZeroX::KeyState::PRESSED_THIS_FRAME );
                break;
            }
            default:
                break;
            }

            const uint64_t clearMask            = ~mask;
            const uint64_t clearedPackedState   = ( *packedState ) & clearMask;

            *packedState = ( clearedPackedState | ( static_cast<uint64_t>(nextState) << ( button * 4 ) ) );
        }
    }

    void updateGamePad(ZeroX::GamePad* gamePad, const GLFWgamepadstate& state, const float* axes, int axesCount)
    {
        updateButtonState(&gamePad->packedState, state);

        if (axesCount < 6)
        {
            return;
        }

        // TODO(Karim): Make this configurable
        constexpr float joystickDeadzone = 0.25f;

        gamePad->leftStickInput     = ZeroX::getScaledRadialInput({ axes[0], axes[1] }, joystickDeadzone);
        gamePad->rightStickInput    = ZeroX::getScaledRadialInput({ axes[2], axes[3] }, joystickDeadzone);
        gamePad->leftTriggerInput   = axes[4];
        gamePad->rightTriggerInput  = axes[5];
    }
}

namespace ZeroX
{
    void monitorJoystickConnections()
    {
        for (uint16_t id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id)
        {
            ::handleJoystickEvent(id, glfwJoystickPresent(id) == GLFW_TRUE ? GLFW_CONNECTED : GLFW_DISCONNECTED);
        }

        glfwSetJoystickCallback(::handleJoystickEvent);
    }

    void updateInput(Input* input)
    {
        size_t controllerIndex{ 0 };

        for (uint16_t id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id)
        {
            if ( ( ZeroX::globalControllerState & ( 1 << id ) ) == (1 << id) )
            {
                if (!glfwJoystickIsGamepad(id))
                {
                    continue;
                }

                GLFWgamepadstate state = { };
                if (!glfwGetGamepadState(id, &state))
                {
                    continue;
                }

                int axesCount{ 0 };
                const float* axes = glfwGetJoystickAxes(id, &axesCount);

                switch (controllerIndex)
                {
                case 0:
                    ::updateGamePad(&input->gamePad1, state, axes, axesCount);
                    break;
                case 1:
                    ::updateGamePad(&input->gamePad2, state, axes, axesCount);
                    break;
                case 2:
                    ::updateGamePad(&input->gamePad3, state, axes, axesCount);
                    break;
                case 3:
                    ::updateGamePad(&input->gamePad4, state, axes, axesCount);
                    break;
                default:
                    break;
                }

                ++controllerIndex;
            }
        }
    }
}