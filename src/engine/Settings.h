#pragma once
#include "Window.h"
#include <filesystem>

namespace rd
{

	// 実行環境ごとの設定(Assetsとは分けて保存する)
	struct Settings
	{
		WindowState window;
	};

	// ファイルがない、または壊れている場合は既定値を返す
	Settings LoadSettings(const std::filesystem::path& path);
	// 保存に失敗しても終了処理は止めず、出力ウィンドウへの通知だけ行う
	void SaveSettings(const std::filesystem::path& path, const Settings& settings);

} // namespace rd
