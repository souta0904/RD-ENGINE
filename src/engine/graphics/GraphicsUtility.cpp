#include "GraphicsUtility.h"
#include "Logger.h"
#include "StringUtility.h"
#include <cstdint>
#include <format>
#include <stdexcept>

namespace rd::GraphicsUtility
{

	// HRESULTのエラーメッセージ(UTF-8)を取得
	std::string GetErrorMessage(HRESULT hr)
	{
		wchar_t* buffer = nullptr;
		const DWORD length = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			static_cast<DWORD>(hr),
			0,
			reinterpret_cast<wchar_t*>(&buffer),
			0,
			nullptr);
		if (length == 0)
		{
			return {};
		}

		// 末尾の改行を取り除く
		std::wstring_view message(buffer, length);
		while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' '))
		{
			message.remove_suffix(1);
		}
		std::string result = StringUtility::ToUtf8(message);
		LocalFree(buffer);
		return result;
	}

	// エラーログを出力してstd::runtime_errorを投げる
	void ThrowHResultError(HRESULT hr, std::string_view what)
	{
		const std::string code = std::format("0x{:08X}", static_cast<uint32_t>(hr));
		Logger::Error("{} failed: {} {}", what, code, GetErrorMessage(hr));
		// 例外のメッセージはANSIとして扱われるので、日本語を含むエラーメッセージは入れない
		throw std::runtime_error(std::format("{} failed: {}", what, code));
	}

} // namespace rd::GraphicsUtility
