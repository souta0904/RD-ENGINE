#pragma once
#include <string>
#include <string_view>

namespace rd::StringUtility
{

	// 不正な文字は置換文字(U+FFFD)に変換する
	std::wstring ToWide(std::string_view utf8);
	std::string ToUtf8(std::wstring_view wide);

} // namespace rd::StringUtility
