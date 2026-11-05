#pragma once
#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

namespace rd
{

	enum class LogLevel
	{
		Debug,
		Info,
		Warning,
		Error,
	};

	// 「[2026-11-05 14:23:01.123] [Info] メッセージ」の形式で出力ウィンドウとログファイルへ出力する
	// 複数のスレッドから呼び出せる
	namespace Logger
	{
		// ログファイルを開く
		// 前回のログは最終更新日時を付けた名前(engine-2026-11-05_14-23-01.logなど)で残し、古いものから削除する
		// 開けない場合は出力ウィンドウのみに出力する(Initializeの前とShutdownの後も同様)
		void Initialize(const std::filesystem::path& path);
		void Shutdown();

		// Debugレベルはデバッグビルドのみ出力する
		bool IsEnabled(LogLevel level);
		// messageはUTF-8
		void Write(LogLevel level, std::string_view message);

		template<typename... Args>
		void Debug(std::format_string<Args...> format, Args&&... args)
		{
			if (IsEnabled(LogLevel::Debug))
			{
				Write(LogLevel::Debug, std::format(format, std::forward<Args>(args)...));
			}
		}

		template<typename... Args>
		void Info(std::format_string<Args...> format, Args&&... args)
		{
			if (IsEnabled(LogLevel::Info))
			{
				Write(LogLevel::Info, std::format(format, std::forward<Args>(args)...));
			}
		}

		template<typename... Args>
		void Warning(std::format_string<Args...> format, Args&&... args)
		{
			if (IsEnabled(LogLevel::Warning))
			{
				Write(LogLevel::Warning, std::format(format, std::forward<Args>(args)...));
			}
		}

		template<typename... Args>
		void Error(std::format_string<Args...> format, Args&&... args)
		{
			if (IsEnabled(LogLevel::Error))
			{
				Write(LogLevel::Error, std::format(format, std::forward<Args>(args)...));
			}
		}
	}

} // namespace rd
