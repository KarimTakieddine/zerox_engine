#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace ZeroX
{
    enum class Button : uint8_t
    {
        BUTTON_A            = 0,
        BUTTON_B            = 1,
        BUTTON_X            = 2,
        BUTTON_Y            = 3,
        BUTTON_LB           = 4,
        BUTTON_RB           = 5,
        BUTTON_START        = 6,
        BUTTON_BACK         = 7,
        LEFT_CLICK          = 8,
        RIGHT_CLICK         = 9,
        BUTTON_DPAD_UP      = 10,
        BUTTON_DPAD_DOWN    = 11,
        BUTTON_DPAD_LEFT    = 12,
        BUTTON_DPAD_RIGHT   = 13,
        BUTTON_LAST         = 14,
        BUTTON_UNKNOWN      = 15
    };

    struct GamePad
    {
        glm::vec2 leftStickInput    { 0.0f, 0.0f };
        glm::vec2 rightStickInput   { 0.0f, 0.0f };
        uint64_t packedState        { 0 };
        float leftTriggerInput      { 0.0f };
        float rightTriggerInput     { 0.0f };
    };

    enum class ControllerState : uint64_t
    {
        DISCONNECTED    = 0,
        CONNECTED       = 1ULL << 63
    };

    enum class KeyState : uint8_t
    {
        NONE                = 0,
        PRESSED             = 1,
        RELEASED            = 1 << 1,
        PRESSED_THIS_FRAME  = 1 << 2,
        RELEASED_THIS_FRAME = 1 << 3
    };

    inline constexpr KeyState operator|(KeyState a, KeyState b) { return static_cast<KeyState>(static_cast<std::underlying_type_t<KeyState>>(a) | static_cast<std::underlying_type_t<KeyState>>(b)); }
    inline constexpr KeyState operator&(KeyState a, KeyState b) { return static_cast<KeyState>(static_cast<std::underlying_type_t<KeyState>>(a) & static_cast<std::underlying_type_t<KeyState>>(b)); }
    inline constexpr KeyState operator~(KeyState s) { return static_cast<KeyState>(~(static_cast<std::underlying_type_t<KeyState>>(s))); }
    inline constexpr KeyState& operator|=(KeyState& a, KeyState b) { a = a | b; return a; }
    inline constexpr KeyState& operator&=(KeyState& a, KeyState b) { a = a & b; return a; }

    struct alignas(16) Input
    {
        GamePad gamePad1;
        GamePad gamePad2;
        GamePad gamePad3;
        GamePad gamePad4;
    };

    inline uint16_t globalControllerState{ 0 };

    inline glm::vec2 getScaledRadialInput(const glm::vec2& input, float deadzone)
    {
        const float adjustedDeadzone    = deadzone < 0.0f || deadzone >= 1.0f ? 0.0f : deadzone;
        const float magnitude           = glm::length(input);

        if (magnitude < adjustedDeadzone)
        {
            return { };
        }

        return glm::normalize(input) * ( ( magnitude - adjustedDeadzone ) / ( 1.0f - adjustedDeadzone ) );
    }
    
    void monitorJoystickConnections();
    void updateInput(Input* input);

    inline KeyState getButtonState(uint64_t packedState, Button button)
    {
        return static_cast<KeyState>( packedState & ( 15ULL << ( static_cast<uint64_t>(button) * 4 ) ) );
    }

    inline bool isButtonPressed(const GamePad& gamePad, Button button)
    {
        return ( getButtonState(gamePad.packedState, button) & KeyState::PRESSED ) == KeyState::PRESSED;
    }

    inline bool isButtonPressedThisFrame(const GamePad& gamePad, Button button)
    {
        return ( getButtonState(gamePad.packedState, button) & KeyState::PRESSED_THIS_FRAME ) == KeyState::PRESSED_THIS_FRAME;
    }

    inline bool isButtonReleased(const GamePad& gamePad, Button button)
    {
        return ( getButtonState(gamePad.packedState, button) & KeyState::RELEASED ) == KeyState::RELEASED;
    }

    inline bool isButtonReleasedThisFrame(const GamePad& gamePad, Button button)
    {
        return ( getButtonState(gamePad.packedState, button) & KeyState::RELEASED_THIS_FRAME) == KeyState::RELEASED_THIS_FRAME;
    }
}
