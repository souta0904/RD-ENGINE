#pragma once
#include "DescriptorHeap.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace rd
{

	class CommandQueue;
	class GraphicsDevice;

	// スワップチェインとバックバッファ
	class SwapChain
	{
	public:
		static constexpr uint32_t kBufferCount = 2;
		// バッファはUNORMで作り、RTVでsRGBとして書き込む(フリップモデルはsRGBのバッファを作れない)
		static constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static constexpr DXGI_FORMAT kRtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		// 作成に失敗した場合はstd::runtime_errorを投げる
		// queueは表示に使うDirectキュー
		SwapChain(
			const GraphicsDevice& device,
			CommandQueue& queue,
			DescriptorHeap& rtvHeap,
			HWND hWnd,
			uint32_t width,
			uint32_t height);
		~SwapChain();
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;

		// バッファのサイズを変更する
		// 呼ぶ前にバックバッファを使うコマンドの完了を待っておくこと
		void Resize(uint32_t width, uint32_t height);
		// 表示する(デバイスが失われた場合はstd::runtime_errorを投げる)
		void Present(bool vsync);

		// アクセッサ
		uint32_t GetCurrentBackBufferIndex() const { return mSwapChain->GetCurrentBackBufferIndex(); }
		ID3D12Resource* GetBackBuffer(uint32_t index) const { return mBackBuffers[index].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtv(uint32_t index) const { return mRtvs[index].cpu; }
		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

	private:
		// バックバッファを取得してRTVを作成
		void CreateBackBuffers();

	private:
		ID3D12Device* mDevice = nullptr;
		DescriptorHeap& mRtvHeap;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
		Microsoft::WRL::ComPtr<ID3D12Resource> mBackBuffers[kBufferCount];
		DescriptorHandle mRtvs[kBufferCount];
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
		UINT mFlags = 0;
	};

} // namespace rd
