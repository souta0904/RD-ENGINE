#pragma once

namespace rd::CrashHandler
{

	// 予期せぬ例外(アクセス違反など)やstd::terminateの発生時に、原因をエラーログへ出力するようにする
	// 出力後はOSの既定の処理に任せるのでプロセスは終了する
	// デバッガ接続中はデバッガが先に例外を受け取るので、アクセス違反などは出力されない
	void Install();

} // namespace rd::CrashHandler
