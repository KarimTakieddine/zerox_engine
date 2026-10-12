#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>

#include <glm/vec2.hpp>

namespace ZeroX
{
    struct GamePad
    {
        glm::vec2 axisInput { 0.0f, 0.0f };
        uint64_t packedState{ 0 };
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
        PRESSED_LAST_FRAME  = 1 << 2,
        RELEASED_LAST_FRAME = 1 << 3
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
    
    void monitorJoystickConnections();
    void updateGamePad(GamePad* gamePad, int id);
    void updateInput(Input* input);
}
