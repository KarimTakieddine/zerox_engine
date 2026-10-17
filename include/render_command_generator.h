#pragma once

#include <render_command.h>
#include <transform.h>
#include <material.h>

namespace ZeroX
{
    renderer::Command generateRenderCommand(const renderer::Transform& transform, size_t batchIndex, size_t entityIndex);
    renderer::Command generateRenderCommand(const renderer::Material& material, size_t batchIndex, size_t entityIndex);
}
