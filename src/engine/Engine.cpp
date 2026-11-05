#include "Engine.h"
#include "EngineInfo.h"
#include "Logger.h"
#include <format>
#include <iterator>

namespace rd
{

	namespace
	{
		constexpr const char* kSettingsPath = "saved/settings.json";

		struct Resolution
		{
			uint32_t width;
			uint32_t height;
		};
		constexpr Resolution kResolutions[] = {
			{ 1280, 720 },
			{ 1600, 900 },
			{ 1920, 1080 },
		};
	}

	Engine::Engine()
		: mSettings(LoadSettings(kSettingsPath))
		, mWindow(
			std::format(
				L"{} v{}.{}.{}",
				EngineInfo::kName,
				EngineInfo::kVersionMajor,
				EngineInfo::kVersionMinor,
				EngineInfo::kVersionPatch),
			mSettings.window)
	{
		mWindow.SetKeyDownHandler([this](uint32_t key) { OnKeyDown(key); });
		Logger::Info("Engine initialized");
	}

	// エンジンを実行
	void Engine::Run()
	{
		// ゲームループ
		while (mWindow.ProcessMessages())
		{
			// TODO: D3D12のPresent(VSync)で待てるようになったら消す
			Sleep(1);
		}

		Logger::Info("Engine shutting down");
		mSettings.window = mWindow.GetFinalState();
		SaveSettings(kSettingsPath, mSettings);
	}

	// TODO: InputSystemとエディタのUIができたらそちらへ移す
	void Engine::OnKeyDown(uint32_t key)
	{
		switch (key)
		{
		case VK_F11:
			mWindow.SetFullscreen(!mWindow.IsFullscreen());
			break;
		case VK_F9:
		{
			mResolutionIndex = (mResolutionIndex + 1) % std::size(kResolutions);
			const Resolution& resolution = kResolutions[mResolutionIndex];
			mWindow.SetClientSize(resolution.width, resolution.height);
			break;
		}
		}
	}

} // namespace rd
