#include <cstring>

#include <material.h>
#include <transform.h>

#include "render_command_generator.h"

namespace ZeroX
{
    renderer::Command generateRenderCommand(const ZeroXGame::Transform& transform, size_t batchIndex, size_t entityIndex)
    {
        static_assert(sizeof(ZeroXGame::Transform) == sizeof(renderer::Transform));

        renderer::Command result{ .type = renderer::CommandType::UPDATE_ENTITY_TRANSFORM };

        std::memcpy(result.data, &batchIndex, sizeof(size_t));
        std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
        std::memcpy(result.data + 2 * sizeof(size_t), &transform, sizeof(ZeroXGame::Transform));

        return result;
    }

    renderer::Command generateRenderCommand(const ZeroXGame::Material& material, size_t batchIndex, size_t entityIndex)
    {
        static_assert(sizeof(ZeroXGame::Material) == sizeof(renderer::Material));

        renderer::Command result{ .type = renderer::CommandType::UPDATE_ENTITY_MATERIAL };

        std::memcpy(result.data, &batchIndex, sizeof(size_t));
        std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
        std::memcpy(result.data + 2 * sizeof(size_t), &material, sizeof(ZeroXGame::Material));

        return result;
    }
}