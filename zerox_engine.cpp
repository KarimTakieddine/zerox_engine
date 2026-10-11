#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <renderer.hpp>
#include <render_frame_buffer.h>
#include <mesh.hpp>
#include <shader.h>

#include "game_library_watcher.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
#include "zerox_glfw_window.h"

int main(int argc, char** argv)
{
    std::unique_ptr<ZeroX::EngineLogger> engineLogger = std::make_unique<ZeroX::EngineLogger>();
    ZeroX::globalEngineLogger = engineLogger.get();
    engineLogger->initialize();

    // TODO(Karim): Do we need heap memory here honestly?

    try
    {
        ZeroX::WindowConfig windowConfig = {
            .name               = "ZeroX Engine",
            .width              = 1920,
            .height             = 1080,
            .openGLVersionMajor = 4,
            .openGLVersionMinor = 6,
            .fullscreen         = false,
            .resizeable         = true
        };

        std::unique_ptr<ZeroX::EngineRuntime> engineRuntime = std::make_unique<ZeroX::EngineRuntime>(windowConfig);

        ZeroX::GameConfig gameConfig = {
            .gameLibraryDir     = "/home/mouns/Repositories/zerox_game/build",
            .gameLibraryFile    = "libzerox_game.so",
            .maxQueuedFrames    = 4
        };

        ZeroX::AudioSound audioSounds[1] = {
            { "./audio/character_jump.mp3", ZeroX::SoundType::SOUND_EFFECT }
        };

        ZeroX::AudioConfig audioConfig = {
            .sounds     = audioSounds,
            .soundCount = 1
        };

        engineRuntime->run(gameConfig, audioConfig);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Engine runtime initialization failure: " << e.what();
    }

    engineLogger->processMessages();
    engineLogger->terminate();

    return 0;
}