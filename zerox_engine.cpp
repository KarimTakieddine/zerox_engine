#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <renderer.hpp>
#include <render_frame_buffer.h>
#include <mesh.hpp>
#include <shader.h>

#include "game_library_watcher.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"

int main(int argc, char** argv)
{
    std::unique_ptr<ZeroX::EngineLogger> engineLogger = std::make_unique<ZeroX::EngineLogger>();
    ZeroX::globalEngineLogger = engineLogger.get();

    ZeroX::EngineRuntime engineRuntime;

    renderer::FrameBuffer renderFrameBuffer;
    renderFrameBuffer.allocate(64);

    engineRuntime.startGameThread({ "/home/mouns/Repositories/zerox_game/build", "libzerox_game.so", 4 }, &renderFrameBuffer);
    engineRuntime.startRenderThread(&renderFrameBuffer);
    engineLogger->initialize();

    engineRuntime.waitForWindowClose();

    engineRuntime.stopGameThread();

    engineLogger->terminate();

    return 0;
}