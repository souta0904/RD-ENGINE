#include "Engine.h"
#include "EngineInfo.h"
#include <exception>
#include <string>

namespace
{
	// 例外のメッセージ(ANSI)をワイド文字列へ変換
	std::wstring ToWide(const char* text)
	{
		const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
		std::wstring result(length, L'\0');
		MultiByteToWideChar(CP_ACP, 0, text, -1, result.data(), length);
		return result;
	}
}

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	// 高DPI環境でOSに拡大されてぼやけないようにする
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	try
	{
		rd::Engine engine;
		engine.Run();
	}
	catch (const std::exception& e)
	{
		// kNameは文字列リテラルなのでnull終端されている
		MessageBox(nullptr, ToWide(e.what()).c_str(), rd::EngineInfo::kName.data(), MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}
