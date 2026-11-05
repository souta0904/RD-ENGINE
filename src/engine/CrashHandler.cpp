#include "CrashHandler.h"
#include "Logger.h"
#include "StringUtility.h"
#include <Windows.h>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>

namespace rd::CrashHandler
{

	namespace
	{
		// ASLRで毎回変わる絶対アドレスの代わりに「モジュール名+オフセット」で表す
		std::string DescribeAddress(const void* address)
		{
			const uintptr_t value = reinterpret_cast<uintptr_t>(address);
			HMODULE module = nullptr;
			wchar_t path[MAX_PATH] = {};
			if (GetModuleHandleExW(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				static_cast<LPCWSTR>(address),
				&module) &&
				GetModuleFileNameW(module, path, MAX_PATH) > 0)
			{
				const std::filesystem::path fileName = std::filesystem::path(path).filename();
				return std::format(
					"{}+0x{:X}",
					StringUtility::ToUtf8(fileName.native()),
					value - reinterpret_cast<uintptr_t>(module));
			}
			return std::format("0x{:X}", value);
		}

		LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* info)
		{
			const EXCEPTION_RECORD& record = *info->ExceptionRecord;
			Logger::Error(
				"Unhandled exception: code 0x{:08X} at {}",
				record.ExceptionCode,
				DescribeAddress(record.ExceptionAddress));
			return EXCEPTION_CONTINUE_SEARCH;
		}

		[[noreturn]] void OnTerminate()
		{
			std::string reason = "no active exception";
			if (const std::exception_ptr exception = std::current_exception())
			{
				try
				{
					std::rethrow_exception(exception);
				}
				catch (const std::exception& e)
				{
					reason = e.what();
				}
				catch (...)
				{
					reason = "unknown exception";
				}
			}
			Logger::Error("std::terminate called: {}", reason);
			std::abort();
		}
	}

	void Install()
	{
		SetUnhandledExceptionFilter(OnUnhandledException);
		std::set_terminate(OnTerminate);
		Logger::Info("Crash handler installed");
	}

} // namespace rd::CrashHandler
