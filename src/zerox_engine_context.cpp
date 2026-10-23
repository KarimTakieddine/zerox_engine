#include <filesystem>
#include <stdexcept>

#include <platform.h>

#include "config/zerox_config_loader.h"
#include "zerox_engine_context.h"
#include "zerox_glfw_window.h"
#include "zerox_miniaudio_player.h"

namespace ZeroX
{
	EngineContext::EngineContext()
	{
		if (glfwInit() != GLFW_TRUE)
		{
			const char* errorBuffer{ nullptr };

			glfwGetError(&errorBuffer);

			throw std::runtime_error(
				std::string("Could not create engine context. Failed to initialize GLFW. Error was: ")
				+ ( errorBuffer ? std::string(errorBuffer) : std::string() ));
		}
	}

	EngineContext::~EngineContext()
	{
		waitForLoadFinish();

		m_gameLibraryWatcher.reset();
		m_gameLibrary.reset();
		m_audioPlayer.reset();
		m_window.reset();

		glfwTerminate();
	}

	void EngineContext::waitForLoadFinish()
	{
		if (m_gameLibraryThread.joinable())
		{
			m_gameLibraryThread.join();
		}

		if (m_audioPlayerThread.joinable())
		{
			m_audioPlayerThread.join();
		}
	}

	void EngineContext::load(const char* configPath, AsyncLoadPromise promise)
	{
		pugi::xml_document configDocument;
		pugi::xml_parse_result parseResult = configDocument.load_file(configPath, pugi::parse_minimal, pugi::encoding_utf8);
		if (parseResult)
		{
			const pugi::xml_node root = configDocument.root().first_child();

			auto windowConfigFuture = loadWindowConfig(root.child("Window"));
			auto audioConfigFuture	= loadAudioConfig(root.child("Audio"));
			auto gameConfigFuture	= loadGameConfig(root.child("Game"));

			m_gameLibraryThread = std::thread(&EngineContext::loadGameLibrary, this, gameConfigFuture.get(), std::move(promise.gameLibrary), std::move(promise.gameLibraryWatcher));
			m_audioPlayerThread = std::thread(&EngineContext::loadAudioPlayer, this, audioConfigFuture.get(), std::move(promise.audioPlayer));

			m_window = std::make_unique<GLFWWindow>(windowConfigFuture.get());
			if (!m_window)
			{
				const char* errorBuffer{ nullptr };
				glfwGetError(&errorBuffer);
				promise.window.set_exception(
					std::make_exception_ptr(
						std::runtime_error(
							std::string("Failed to initialize GLFWWindow. Error was: ")
							+ (errorBuffer ? std::string(errorBuffer) : std::string()))));
				return;
			}
			promise.window.set_value(m_window.get());
		}
		else
		{
			m_gameLibraryThread = std::thread(&EngineContext::loadGameLibrary, this, GameConfig{ }, std::move(promise.gameLibrary), std::move(promise.gameLibraryWatcher));
			m_audioPlayerThread = std::thread(&EngineContext::loadAudioPlayer, this, AudioConfig{ }, std::move(promise.audioPlayer));

			m_window = std::make_unique<GLFWWindow>(WindowConfig{ });
			if (!m_window)
			{
				const char* errorBuffer{ nullptr };
				glfwGetError(&errorBuffer);
				promise.window.set_exception(
					std::make_exception_ptr(
						std::runtime_error(
							std::string("Failed to initialize GLFWWindow. Error was: ")
							+ (errorBuffer ? std::string(errorBuffer) : std::string()))));
				return;
			}
			promise.window.set_value(m_window.get());
		}
	}

	void EngineContext::loadAudioPlayer(const AudioConfig& audioConfig, std::promise<IAudioPlayer*> promise)
	{
		m_audioPlayer = std::make_unique<MiniAudioPlayer>();

		if (!m_audioPlayer->initialize(audioConfig))
		{
			promise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to initialize MiniAudio player")));
			return;
		}

		promise.set_value(m_audioPlayer.get());
	}

	void EngineContext::loadGameLibrary(const GameConfig& gameConfig, std::promise<GameLibrary*> libraryPromise, std::promise<GameLibraryWatcher*> watcherPromise)
	{
		m_gameLibrary = std::make_unique<GameLibrary>();

		const std::filesystem::path gameDirectoryPath(gameConfig.gameLibraryDir);
		if (!std::filesystem::is_directory(gameDirectoryPath))
		{
			libraryPromise.set_exception(std::make_exception_ptr(std::runtime_error(std::string("Failed to initialize game library. Is not a directory: ") + gameConfig.gameLibraryDir)));
		}
		else
		{
			const std::filesystem::path gameLibraryPath = gameDirectoryPath / gameConfig.gameLibraryFile;
			if (!std::filesystem::exists(gameLibraryPath) || !std::filesystem::is_regular_file(gameLibraryPath))
			{
				libraryPromise.set_exception(std::make_exception_ptr(std::runtime_error(std::string("Failed to initialize game library. Is not a file: ") + gameLibraryPath.string())));
			}
			else if (!m_gameLibrary->load(gameLibraryPath.string().c_str()))
			{
				libraryPromise.set_exception(std::make_exception_ptr(std::runtime_error(std::string("Failed to initialize game library. Could not load functions from") + gameLibraryPath.string())));
			}
			else
			{
				libraryPromise.set_value(m_gameLibrary.get());
			}
		}

		m_gameLibraryWatcher = std::make_unique<GameLibraryWatcher>();
		
		if (m_gameLibraryWatcher->startWatching(gameConfig.gameLibraryDir.c_str(), gameConfig.gameLibraryFile.c_str()))
		{
			watcherPromise.set_value(m_gameLibraryWatcher.get());
		}
		else
		{
			watcherPromise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to initialize game library watcher")));
		}
	}
}