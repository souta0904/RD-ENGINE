#include "Settings.h"
#include "JsonUtility.h"
#include <algorithm>

namespace rd
{

	using json = nlohmann::json;

	namespace
	{
		// 手で編集された値でもウィンドウ計算が破綻しない範囲
		constexpr uint32_t kMinClientSize = 64;
		constexpr uint32_t kMaxClientSize = 16384;
		constexpr int32_t kMaxPosition = 100000;
	}

	Settings LoadSettings(const std::filesystem::path& path)
	{
		Settings settings;
		const std::optional<json> root = JsonUtility::LoadFile(path);
		if (!root)
		{
			return settings;
		}
		const json* window = JsonUtility::FindObject(*root, "window");
		if (!window)
		{
			return settings;
		}

		// 読めなかった項目は既定値のまま
		WindowState& state = settings.window;
		JsonUtility::Get(*window, "width", state.width);
		JsonUtility::Get(*window, "height", state.height);
		state.width = std::clamp(state.width, kMinClientSize, kMaxClientSize);
		state.height = std::clamp(state.height, kMinClientSize, kMaxClientSize);

		int32_t x = 0;
		int32_t y = 0;
		if (JsonUtility::Get(*window, "x", x) && JsonUtility::Get(*window, "y", y))
		{
			state.position = POINT{
				std::clamp(x, -kMaxPosition, kMaxPosition),
				std::clamp(y, -kMaxPosition, kMaxPosition) };
		}

		JsonUtility::Get(*window, "maximized", state.maximized);
		JsonUtility::Get(*window, "fullscreen", state.fullscreen);
		return settings;
	}

	void SaveSettings(const std::filesystem::path& path, const Settings& settings)
	{
		const WindowState& state = settings.window;
		json root;
		json& window = root["window"];
		JsonUtility::Set(window, "width", state.width);
		JsonUtility::Set(window, "height", state.height);
		if (state.position)
		{
			JsonUtility::Set(window, "x", state.position->x);
			JsonUtility::Set(window, "y", state.position->y);
		}
		JsonUtility::Set(window, "maximized", state.maximized);
		JsonUtility::Set(window, "fullscreen", state.fullscreen);

		JsonUtility::SaveFile(path, root);
	}

} // namespace rd
