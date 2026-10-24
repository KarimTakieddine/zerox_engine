#pragma once

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <zerox_input.h>

namespace ZeroX
{
    using ZeroXBridge::Input;

    inline uint16_t globalControllerState   { 0 };
    inline uint64_t globalControllerMappings{ 0 };

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
}
