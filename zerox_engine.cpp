#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <renderer.hpp>
#include <render_frame_buffer.h>
#include <mesh.hpp>
#include <shader.h>

#include "game_library_watcher.h"
#include "zerox_engine_runtime.h"

int main(int argc, char** argv)
{
    ZeroX::EngineRuntime engineRuntime;

    renderer::FrameBuffer renderFrameBuffer;
    renderFrameBuffer.allocate(64);

    engineRuntime.startGameThread("/home/mouns/Repositories/zerox_game/build", "libzerox_game.so", &renderFrameBuffer);

    engineRuntime.startRenderThread(&renderFrameBuffer);
    engineRuntime.waitForWindowClose();

    engineRuntime.stopGameThread();

    return 0;
}