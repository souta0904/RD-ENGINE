#include "Settings.h"
#include <algorithm>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

namespace rd
{

	using json = nlohmann::json;

	namespace
	{
		// 手で編集された値でもウィンドウ計算が破綻しない範囲
		constexpr int kMinClientSize = 64;
		constexpr int kMaxClientSize = 16384;
		constexpr int kMaxPosition = 100000;

		void Log(const std::string& message)
		{
			OutputDebugStringA((message + "\n").c_str());
		}
	}

	Settings LoadSettings(const std::filesystem::path& path)
	{
		Settings settings;
		std::ifstream file(path);
		if (!file)
		{
			// 初回起動
			return settings;
		}

		try
		{
			const json root = json::parse(file);
			const json& window = root.at("window");
			WindowState state;
			state.width = std::clamp(window.at("width").get<int>(), kMinClientSize, kMaxClientSize);
			state.height = std::clamp(window.at("height").get<int>(), kMinClientSize, kMaxClientSize);
			state.position = POINT{
				std::clamp(window.at("x").get<int>(), -kMaxPosition, kMaxPosition),
				std::clamp(window.at("y").get<int>(), -kMaxPosition, kMaxPosition) };
			state.maximized = window.at("maximized").get<bool>();
			state.fullscreen = window.at("fullscreen").get<bool>();
			settings.window = state;
		}
		catch (const json::exception& e)
		{
			Log(std::format("Failure: Load settings: {}", e.what()));
		}
		return settings;
	}

	void SaveSettings(const std::filesystem::path& path, const Settings& settings)
	{
		const WindowState& state = settings.window;
		const POINT position = state.position.value_or(POINT{ 0, 0 });
		const json root = {
			{ "window", {
				{ "width", state.width },
				{ "height", state.height },
				{ "x", position.x },
				{ "y", position.y },
				{ "maximized", state.maximized },
				{ "fullscreen", state.fullscreen } } } };

		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);

		std::ofstream file(path);
		file << root.dump(1, '\t') << '\n';
		if (!file)
		{
			Log("Failure: Save settings");
		}
	}

} // namespace rd
