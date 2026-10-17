#include <cstring>

#include "render_command_generator.h"

namespace ZeroX
{
    renderer::Command generateRenderCommand(const renderer::Transform& transform, size_t batchIndex, size_t entityIndex)
    {
        renderer::Command result{ .type = renderer::CommandType::UPDATE_ENTITY_TRANSFORM };

        std::memcpy(result.data, &batchIndex, sizeof(size_t));
        std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
        std::memcpy(result.data + 2 * sizeof(size_t), &transform, sizeof(renderer::Transform));

        return result;
    }

    renderer::Command generateRenderCommand(const renderer::Material& material, size_t batchIndex, size_t entityIndex)
    {
        renderer::Command result{ .type = renderer::CommandType::UPDATE_ENTITY_MATERIAL };

        std::memcpy(result.data, &batchIndex, sizeof(size_t));
        std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
        std::memcpy(result.data + 2 * sizeof(size_t), &material, sizeof(renderer::Material));

        return result;
    }
}