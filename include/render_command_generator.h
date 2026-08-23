#pragma once

#include <zerox_game_memory.hpp>
#include <render_command.h>

namespace ZeroX
{
    renderer::Command generateRenderCommand(const ZeroXGame::Transform& transform, size_t batchIndex, size_t entityIndex);
    renderer::Command generateRenderCommand(const ZeroXGame::Material& material, size_t batchIndex, size_t entityIndex);
}
