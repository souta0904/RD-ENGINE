#pragma once
#include "Settings.h"
#include "Window.h"
#include <cstddef>
#include <cstdint>

namespace rd
{

	// エンジン
	class Engine
	{
	public:
		Engine();

		// エンジンを実行
		void Run();

	private:
		void OnKeyDown(uint32_t key);

	private:
		// mWindowの初期化に使うので先に宣言する
		Settings mSettings;
		Window mWindow;
		size_t mResolutionIndex = 0;
	};

} // namespace rd
