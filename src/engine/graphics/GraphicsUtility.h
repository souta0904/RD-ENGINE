#pragma once
#include <Windows.h>
#include <string>
#include <string_view>

namespace rd::GraphicsUtility
{

	// HRESULTのエラーメッセージ(UTF-8)を取得
	std::string GetErrorMessage(HRESULT hr);

	// エラーログを出力してstd::runtime_errorを投げる
	[[noreturn]] void ThrowHResultError(HRESULT hr, std::string_view what);

	// 失敗していたらエラーログを出力してstd::runtime_errorを投げる
	// whatには失敗した処理の名前を渡す
	inline void ThrowIfFailed(HRESULT hr, std::string_view what)
	{
		if (FAILED(hr))
		{
			ThrowHResultError(hr, what);
		}
	}

} // namespace rd::GraphicsUtility
