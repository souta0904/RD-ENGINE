#include "Logger.h"
#include "StringUtility.h"
#include <Windows.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace rd::Logger
{

	namespace
	{
#ifdef _DEBUG
		constexpr LogLevel kMinLevel = LogLevel::Debug;
#else
		constexpr LogLevel kMinLevel = LogLevel::Info;
#endif

		// 残しておく過去のログの数
		constexpr size_t kMaxBackupCount = 10;

		std::mutex gMutex;
		std::ofstream gFile;

		const char* ToString(LogLevel level)
		{
			switch (level)
			{
			case LogLevel::Debug:
				return "Debug";
			case LogLevel::Info:
				return "Info";
			case LogLevel::Warning:
				return "Warning";
			case LogLevel::Error:
				return "Error";
			}
			return "Unknown";
		}

		// ファイルの最終更新日時(ローカル時刻)を取得
		SYSTEMTIME GetLastWriteLocalTime(const std::filesystem::path& path)
		{
			WIN32_FILE_ATTRIBUTE_DATA data = {};
			SYSTEMTIME utc = {};
			SYSTEMTIME local = {};
			if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data) &&
				FileTimeToSystemTime(&data.ftLastWriteTime, &utc) &&
				SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local))
			{
				return local;
			}
			GetLocalTime(&local);
			return local;
		}

		// engine.log -> engine-2026-11-05_14-23-01.log
		std::filesystem::path MakeBackupPath(const std::filesystem::path& path)
		{
			const SYSTEMTIME time = GetLastWriteLocalTime(path);
			const std::wstring fileName = std::format(
				L"{}-{:04}-{:02}-{:02}_{:02}-{:02}-{:02}{}",
				path.stem().native(),
				time.wYear,
				time.wMonth,
				time.wDay,
				time.wHour,
				time.wMinute,
				time.wSecond,
				path.extension().native());
			return path.parent_path() / fileName;
		}

		// 古いバックアップを削除
		void RemoveOldBackups(const std::filesystem::path& path)
		{
			namespace fs = std::filesystem;

			const std::wstring prefix = path.stem().native() + L"-";
			const fs::path folder = path.has_parent_path() ? path.parent_path() : fs::path(L".");

			struct Backup
			{
				fs::file_time_type time;
				fs::path path;
			};
			std::vector<Backup> backups;

			std::error_code error;
			for (fs::directory_iterator it(folder, error), end; !error && it != end; it.increment(error))
			{
				const fs::path& candidate = it->path();
				if (candidate.extension() == path.extension() && candidate.stem().native().starts_with(prefix))
				{
					std::error_code timeError;
					const fs::file_time_type time = fs::last_write_time(candidate, timeError);
					if (!timeError)
					{
						backups.push_back({ time, candidate });
					}
				}
			}

			std::ranges::sort(backups, std::greater{}, &Backup::time);
			for (size_t i = kMaxBackupCount; i < backups.size(); ++i)
			{
				fs::remove(backups[i].path, error);
			}
		}

		// 前回のログをバックアップとして残す
		void RotateLogFile(const std::filesystem::path& path)
		{
			std::error_code error;
			if (std::filesystem::is_regular_file(path, error))
			{
				std::filesystem::rename(path, MakeBackupPath(path), error);
			}
			RemoveOldBackups(path);
		}
	}

	void Initialize(const std::filesystem::path& path)
	{
		bool isOpen = false;
		{
			const std::scoped_lock lock(gMutex);
			std::error_code error;
			if (path.has_parent_path())
			{
				std::filesystem::create_directories(path.parent_path(), error);
			}
			RotateLogFile(path);
			gFile.close();
			gFile.clear();
			// 多重起動などで退避できなかった場合に既存のログを消さないよう、追記で開く
			gFile.open(path, std::ios::app);
			isOpen = gFile.is_open();
		}

		const std::string pathText = StringUtility::ToUtf8(path.native());
		if (isOpen)
		{
			Info("Logger initialized: {}", pathText);
		}
		else
		{
			Warning("Failed to open log file: {}", pathText);
		}
	}

	void Shutdown()
	{
		const std::scoped_lock lock(gMutex);
		gFile.close();
	}

	bool IsEnabled(LogLevel level)
	{
		return level >= kMinLevel;
	}

	void Write(LogLevel level, std::string_view message)
	{
		SYSTEMTIME time = {};
		GetLocalTime(&time);
		const std::string line = std::format(
			"[{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}] [{}] {}\n",
			time.wYear,
			time.wMonth,
			time.wDay,
			time.wHour,
			time.wMinute,
			time.wSecond,
			time.wMilliseconds,
			ToString(level),
			message);

		const std::scoped_lock lock(gMutex);
		OutputDebugStringW(StringUtility::ToWide(line).c_str());
		if (gFile.is_open())
		{
			// クラッシュしても直前までのログが残るように毎回書き出す
			gFile << line;
			gFile.flush();
		}
	}

} // namespace rd::Logger
