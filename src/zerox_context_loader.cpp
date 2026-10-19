#include <filesystem>

#include <platform.h>

#include "config/zerox_config_loader.h"
#include "zerox_context_loader.h"
#include "zerox_glfw_window.h"
#include "zerox_miniaudio_player.h"

namespace ZeroX
{
	ContextLoader::ContextLoader() : m_configLoader(std::make_unique<ConfigLoader>()), m_engineContext(std::make_unique<EngineContext>()) { }

	ContextLoader::~ContextLoader()
	{
		if (m_gameLoadThread.joinable())
		{
			m_gameLoadThread.join();
		}

		if (m_audioLoadThread.joinable())
		{
			m_audioLoadThread.join();
		}
	}

	std::future<WindowConfig> ContextLoader::getWindowConfigFuture()
	{
		return m_configLoader->getWindowConfigFuture();
	}

	std::future<IAudioPlayer*> ContextLoader::getAudioPlayerFuture()
	{
		return m_audioPlayerPromise.get_future();
	}

	std::future<GameLibrary*> ContextLoader::getGameLibraryFuture()
	{
		return m_gameLibraryPromise.get_future();
	}

	std::future<GameLibraryWatcher*> ContextLoader::getGameLibraryWatcherFuture()
	{
		return m_gameLibraryWatcherPromise.get_future();
	}

	void ContextLoader::load(const char* configFile)
	{
		auto audioConfigFuture	= m_configLoader->getAudioConfigFuture();
		auto gameConfigFuture 	= m_configLoader->getGameConfigFuture();

		m_configLoader->load(configFile);

		try
		{
			m_audioLoadThread	= std::thread(&ContextLoader::loadAudio, this, audioConfigFuture.get());
			m_gameLoadThread	= std::thread(&ContextLoader::loadGame, this, gameConfigFuture.get());
		}
		catch (const std::exception&)
		{
			auto e = std::current_exception();

			m_audioPlayerPromise.set_exception(e);
			m_gameLibraryPromise.set_exception(e);
			m_gameLibraryWatcherPromise.set_exception(e);
		}
	}

	EngineContext* ContextLoader::getEngineContext() const
	{
		return m_engineContext.get();
	}

	void ContextLoader::loadAudio(const AudioConfig& config)
	{
		m_engineContext->audioPlayer = std::make_unique<MiniAudioPlayer>();

		if (!m_engineContext->audioPlayer->initialize(config))
		{
			m_audioPlayerPromise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to initialize audio player")));
		}
		else
		{
			m_audioPlayerPromise.set_value(m_engineContext->audioPlayer.get());
		}
	}

	void ContextLoader::loadGame(const GameConfig& config)
	{
		const std::filesystem::path gameLibraryDirPath(config.gameLibraryDir);

		const std::filesystem::path gameLibraryPath = gameLibraryDirPath / config.gameLibraryFile;

		if (!std::filesystem::exists(gameLibraryPath))
		{
			auto e = std::make_exception_ptr(std::runtime_error(std::string("Failed to initialize game library: Is not a file: ") + gameLibraryPath.string()));

			m_gameLibraryPromise.set_exception(e);
			m_gameLibraryWatcherPromise.set_exception(e);

			return;
		}

		// TODO(Karim): This needs fixing!
		const std::filesystem::path copyPath = gameLibraryDirPath / (std::string(config.gameLibraryFile) + '_' + std::to_string(0));
		if (!platform::copyFile(gameLibraryPath.string().c_str(), copyPath.string().c_str()))
		{
			auto e = std::make_exception_ptr(std::runtime_error(std::string("Failed to copy game library file to: ") + copyPath.string()));

			m_gameLibraryPromise.set_exception(e);
			m_gameLibraryWatcherPromise.set_exception(e);

			return;
		}

		m_engineContext->gameLibrary = std::make_unique<GameLibrary>();

		if (!m_engineContext->gameLibrary->load(copyPath.string().c_str()))
		{
			auto e = std::make_exception_ptr(std::runtime_error(std::string("Failed to load game library from: ") + copyPath.string()));
			
			m_gameLibraryPromise.set_exception(e);
			m_gameLibraryWatcherPromise.set_exception(e);

			return;
		}

		m_gameLibraryPromise.set_value(m_engineContext->gameLibrary.get());

		const std::filesystem::path gameDirectoryPath(config.gameLibraryDir);
		if (!std::filesystem::is_directory(gameDirectoryPath))
		{
			auto e = std::make_exception_ptr(std::runtime_error(std::string("Failed to start watching game library: ") + gameDirectoryPath.string() + " is not a directory"));

			m_gameLibraryWatcherPromise.set_exception(e);
		}

		m_engineContext->gameLibraryWatcher = std::make_unique<GameLibraryWatcher>();

		if (!m_engineContext->gameLibraryWatcher->startWatching(gameDirectoryPath.string().c_str(), config.gameLibraryFile.c_str()))
		{
			auto e = std::make_exception_ptr(std::runtime_error(std::string("Failed to start watching game library")));

			m_gameLibraryWatcherPromise.set_exception(e);
		}

		m_gameLibraryWatcherPromise.set_value(m_engineContext->gameLibraryWatcher.get());
	}
}