#include "Window.h"
#include "Logger.h"
#include <format>
#include <stdexcept>

namespace rd
{

	namespace
	{
		constexpr const wchar_t* kClassName = L"RdWndClass";
		constexpr DWORD kWindowedStyle = WS_OVERLAPPEDWINDOW;

		// ウィンドウの枠の分のサイズを取得
		SIZE GetFrameSize(HWND hWnd)
		{
			RECT rc = {};
			AdjustWindowRectExForDpi(&rc, kWindowedStyle, FALSE, 0, GetDpiForWindow(hWnd));
			return { rc.right - rc.left, rc.bottom - rc.top };
		}

		// 左上とクライアント領域のサイズからウィンドウの矩形を計算
		RECT MakeWindowRect(HWND hWnd, LONG left, LONG top, uint32_t width, uint32_t height)
		{
			const SIZE frame = GetFrameSize(hWnd);
			return {
				left,
				top,
				left + static_cast<LONG>(width) + frame.cx,
				top + static_cast<LONG>(height) + frame.cy };
		}
	}

	LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// CreateWindowに渡したthisをウィンドウに紐づける
		if (msg == WM_NCCREATE)
		{
			auto create = reinterpret_cast<const CREATESTRUCT*>(lParam);
			auto window = static_cast<Window*>(create->lpCreateParams);
			window->mHWnd = hWnd;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}

		auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (window)
		{
			return window->HandleMessage(msg, wParam, lParam);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_SIZE:
		{
			const uint32_t width = LOWORD(lParam);
			const uint32_t height = HIWORD(lParam);
			mIsMinimized = wParam == SIZE_MINIMIZED || width == 0 || height == 0;
			if (!mIsMinimized)
			{
				mWidth = width;
				mHeight = height;
			}
			return 0;
		}
		case WM_KEYDOWN:
		{
			// bit30: 直前もキーが押されていた(リピート)
			const bool isRepeat = (lParam & (1 << 30)) != 0;
			if (!isRepeat && mKeyDownHandler)
			{
				mKeyDownHandler(static_cast<uint32_t>(wParam));
			}
			return 0;
		}
		case WM_DESTROY:
			// この後はウィンドウの情報を取得できないのでここで保存しておく
			mFinalState = CaptureState();
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(mHWnd, msg, wParam, lParam);
	}

	Window::Window(const std::wstring& title, const WindowState& state)
		: mHInst(GetModuleHandle(nullptr))
		, mWidth(state.width)
		, mHeight(state.height)
	{
		// ウィンドウクラスを登録
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = WndProc;
		wc.hInstance = mHInst;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wc.lpszClassName = kClassName;
		if (!RegisterClassEx(&wc))
		{
			throw std::runtime_error(std::format("RegisterClassEx failed: {}", GetLastError()));
		}

		// 枠のサイズは配置先のモニタのDPIで変わるので、作成してからサイズを決める
		const int x = state.position ? state.position->x : CW_USEDEFAULT;
		const int y = state.position ? state.position->y : CW_USEDEFAULT;
		const HWND hWnd = CreateWindow(
			kClassName,
			title.c_str(),
			kWindowedStyle,
			x,
			y,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			mHInst,
			this);
		if (!hWnd)
		{
			const DWORD error = GetLastError();
			// コンストラクタで例外を投げるとデストラクタは呼ばれないのでここで解除
			UnregisterClass(kClassName, mHInst);
			throw std::runtime_error(std::format("CreateWindow failed: {}", error));
		}

		// 配置を復元して表示
		// SetWindowPlacementは画面外の座標を画面内に収めてくれる
		WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(mHWnd, &placement);
		placement.rcNormalPosition = MakeWindowRect(
			mHWnd,
			placement.rcNormalPosition.left,
			placement.rcNormalPosition.top,
			state.width,
			state.height);
		placement.flags = 0;
		placement.showCmd = state.maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
		if (state.fullscreen)
		{
			// 通常表示が一瞬見えないように、表示する前にフルスクリーンにする
			mRestorePlacement = placement;
			placement.showCmd = SW_HIDE;
			SetWindowPlacement(mHWnd, &placement);
			ApplyFullscreenStyle();
			mIsFullscreen = true;
			ShowWindow(mHWnd, SW_SHOW);
		}
		else
		{
			SetWindowPlacement(mHWnd, &placement);
		}

		Logger::Info(
			"Window created: {}x{}, DPI {}, maximized {}, fullscreen {}",
			mWidth,
			mHeight,
			GetDpiForWindow(mHWnd),
			state.maximized,
			state.fullscreen);
	}

	Window::~Window()
	{
		// ×ボタンで閉じた場合は破棄済み
		if (IsWindow(mHWnd))
		{
			DestroyWindow(mHWnd);
		}
		UnregisterClass(kClassName, mHInst);
		Logger::Info("Window destroyed");
	}

	// 溜まっているメッセージをすべて処理
	bool Window::ProcessMessages()
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}

	// クライアント領域のサイズを変更して通常表示に戻す
	void Window::SetClientSize(uint32_t width, uint32_t height)
	{
		WINDOWPLACEMENT placement = mRestorePlacement;
		if (!mIsFullscreen)
		{
			GetWindowPlacement(mHWnd, &placement);
		}
		placement.rcNormalPosition = MakeWindowRect(
			mHWnd,
			placement.rcNormalPosition.left,
			placement.rcNormalPosition.top,
			width,
			height);
		placement.flags = 0;
		placement.showCmd = SW_SHOWNORMAL;

		if (mIsFullscreen)
		{
			mRestorePlacement = placement;
		}
		else
		{
			SetWindowPlacement(mHWnd, &placement);
		}
		Logger::Info("Window client size set: {}x{}", width, height);
	}

	// ボーダーレスフルスクリーンを切り替え
	void Window::SetFullscreen(bool fullscreen)
	{
		if (fullscreen == mIsFullscreen)
		{
			return;
		}

		if (fullscreen)
		{
			GetWindowPlacement(mHWnd, &mRestorePlacement);
			ApplyFullscreenStyle();
		}
		else
		{
			const LONG_PTR style = GetWindowLongPtr(mHWnd, GWL_STYLE);
			SetWindowLongPtr(mHWnd, GWL_STYLE, style | kWindowedStyle);
			SetWindowPlacement(mHWnd, &mRestorePlacement);
			// スタイルの変更を反映
			SetWindowPos(
				mHWnd, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
		mIsFullscreen = fullscreen;
		Logger::Info("Window fullscreen: {}", fullscreen);
	}

	// 枠を消してモニタ全体に広げる
	void Window::ApplyFullscreenStyle()
	{
		MONITORINFO monitor = { sizeof(MONITORINFO) };
		GetMonitorInfo(MonitorFromWindow(mHWnd, MONITOR_DEFAULTTONEAREST), &monitor);

		const LONG_PTR style = GetWindowLongPtr(mHWnd, GWL_STYLE);
		SetWindowLongPtr(mHWnd, GWL_STYLE, style & ~static_cast<LONG_PTR>(kWindowedStyle));

		const RECT& rc = monitor.rcMonitor;
		SetWindowPos(
			mHWnd, HWND_TOP,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}

	// 現在の状態を取得
	WindowState Window::CaptureState() const
	{
		WINDOWPLACEMENT placement = mRestorePlacement;
		if (!mIsFullscreen)
		{
			GetWindowPlacement(mHWnd, &placement);
		}

		const RECT& rc = placement.rcNormalPosition;
		const SIZE frame = GetFrameSize(mHWnd);

		WindowState state;
		state.width = static_cast<uint32_t>(rc.right - rc.left - frame.cx);
		state.height = static_cast<uint32_t>(rc.bottom - rc.top - frame.cy);
		state.position = POINT{ rc.left, rc.top };
		// 最大化から最小化した場合も最大化として保存
		state.maximized =
			placement.showCmd == SW_SHOWMAXIMIZED ||
			(placement.showCmd == SW_SHOWMINIMIZED && (placement.flags & WPF_RESTORETOMAXIMIZED));
		state.fullscreen = mIsFullscreen;
		return state;
	}

} // namespace rd
