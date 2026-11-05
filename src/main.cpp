#include "CrashHandler.h"
#include "Engine.h"
#include "EngineInfo.h"
#include "Logger.h"
#include "StringUtility.h"
#include <exception>
#include <string>

namespace
{
	constexpr const char* kLogPath = "saved/logs/engine.log";

#ifdef _DEBUG
	constexpr const char* kBuildConfig = "Debug";
#else
	constexpr const char* kBuildConfig = "Release";
#endif

	// 例外のメッセージ(ANSI)をワイド文字列へ変換
	std::wstring AnsiToWide(const char* text)
	{
		const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
		std::wstring result(length, L'\0');
		MultiByteToWideChar(CP_ACP, 0, text, -1, result.data(), length);
		return result;
	}
}

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	rd::Logger::Initialize(kLogPath);
	rd::Logger::Info(
		"{} v{}.{}.{} ({})",
		rd::StringUtility::ToUtf8(rd::EngineInfo::kName),
		rd::EngineInfo::kVersionMajor,
		rd::EngineInfo::kVersionMinor,
		rd::EngineInfo::kVersionPatch,
		kBuildConfig);
	rd::CrashHandler::Install();

	// 高DPI環境でOSに拡大されてぼやけないようにする
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		rd::Logger::Warning("SetProcessDpiAwarenessContext failed: {}", GetLastError());
	}

	int exitCode = 0;
	try
	{
		rd::Engine engine;
		engine.Run();
	}
	catch (const std::exception& e)
	{
		const std::wstring message = AnsiToWide(e.what());
		// c_str()で渡すので、変換結果のnull終端は取り除いておく
		rd::Logger::Error("Fatal error: {}", rd::StringUtility::ToUtf8(message.c_str()));
		// kNameは文字列リテラルなのでnull終端されている
		MessageBox(nullptr, message.c_str(), rd::EngineInfo::kName.data(), MB_OK | MB_ICONERROR);
		exitCode = 1;
	}
	catch (...)
	{
		rd::Logger::Error("Fatal error: unknown exception");
		MessageBox(nullptr, L"Unknown error", rd::EngineInfo::kName.data(), MB_OK | MB_ICONERROR);
		exitCode = 1;
	}

	rd::Logger::Info("Engine exited with code {}", exitCode);
	rd::Logger::Shutdown();
	return exitCode;
}
