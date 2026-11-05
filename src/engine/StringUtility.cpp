#include "StringUtility.h"
#include <Windows.h>

namespace rd::StringUtility
{

	std::wstring ToWide(std::string_view utf8)
	{
		if (utf8.empty())
		{
			return {};
		}

		const int inputSize = static_cast<int>(utf8.size());
		const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, nullptr, 0);
		std::wstring result(size, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, result.data(), size);
		return result;
	}

	std::string ToUtf8(std::wstring_view wide)
	{
		if (wide.empty())
		{
			return {};
		}

		const int inputSize = static_cast<int>(wide.size());
		const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), inputSize, nullptr, 0, nullptr, nullptr);
		std::string result(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide.data(), inputSize, result.data(), size, nullptr, nullptr);
		return result;
	}

} // namespace rd::StringUtility
