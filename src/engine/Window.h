#pragma once
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace rd
{

	// 保存・復元するウィンドウの状態
	struct WindowState
	{
		// 通常表示時のクライアント領域のサイズ
		uint32_t width = 1280;
		uint32_t height = 720;
		// 通常表示時のウィンドウの左上(なければOSに任せる)
		std::optional<POINT> position;
		bool maximized = false;
		bool fullscreen = false;
	};

	// ウィンドウ
	class Window
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		Window(const std::wstring& title, const WindowState& state);
		~Window();
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		// 溜まっているメッセージをすべて処理
		// 終了要求(WM_QUIT)を受け取ったらfalseを返す
		bool ProcessMessages();

		// クライアント領域のサイズを変更して通常表示に戻す
		// フルスクリーン中は解除したときに反映される
		void SetClientSize(uint32_t width, uint32_t height);
		// ボーダーレスフルスクリーンを切り替え
		void SetFullscreen(bool fullscreen);

		// キーが押されたときに呼ぶ関数を設定(長押しのリピートでは呼ばない)
		// TODO: InputSystemを移植したら消す
		void SetKeyDownHandler(std::function<void(uint32_t)> handler) { mKeyDownHandler = std::move(handler); }

		// アクセッサ
		HWND GetHWnd() const { return mHWnd; }
		// 現在のクライアント領域のサイズ(最小化中は最小化前のサイズ)
		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }
		// 描画できないとき(最小化中、または高さや幅が0)にtrue
		bool IsMinimized() const { return mIsMinimized; }
		bool IsFullscreen() const { return mIsFullscreen; }
		// ウィンドウが破棄された時点の状態
		const WindowState& GetFinalState() const { return mFinalState; }

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

		// 枠を消してモニタ全体に広げる
		void ApplyFullscreenStyle();
		// 現在の状態を取得
		WindowState CaptureState() const;

	private:
		// インスタンスハンドル
		HINSTANCE mHInst = nullptr;
		// ウィンドウハンドル
		HWND mHWnd = nullptr;
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
		bool mIsMinimized = false;
		bool mIsFullscreen = false;
		// フルスクリーン解除時に戻す配置
		WINDOWPLACEMENT mRestorePlacement = { sizeof(WINDOWPLACEMENT) };
		WindowState mFinalState;
		std::function<void(uint32_t)> mKeyDownHandler;
	};

} // namespace rd
