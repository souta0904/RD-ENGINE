#include "SwapChain.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"
#include "GraphicsUtility.h"
#include "Logger.h"
#include <format>

namespace rd
{

	using Microsoft::WRL::ComPtr;
	using GraphicsUtility::ThrowIfFailed;

	SwapChain::SwapChain(
		const GraphicsDevice& device,
		CommandQueue& queue,
		DescriptorHeap& rtvHeap,
		HWND hWnd,
		uint32_t width,
		uint32_t height)
		: mDevice(device.GetDevice())
		, mRtvHeap(rtvHeap)
		, mWidth(width)
		, mHeight(height)
	{
		if (device.IsTearingSupported())
		{
			mFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Format = kFormat;
		desc.SampleDesc.Count = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = kBufferCount;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = mFlags;

		IDXGIFactory6* factory = device.GetFactory();
		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(
			factory->CreateSwapChainForHwnd(queue.GetQueue(), hWnd, &desc, nullptr, nullptr, &swapChain),
			"CreateSwapChainForHwnd");
		ThrowIfFailed(swapChain.As(&mSwapChain), "QueryInterface(IDXGISwapChain4)");

		// フルスクリーンはWindowで切り替えるので、DXGIのAlt+Enterは無効にする
		factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

		CreateBackBuffers();
		Logger::Info("SwapChain created: {}x{}, {} buffers", width, height, kBufferCount);
	}

	SwapChain::~SwapChain()
	{
		for (DescriptorHandle& rtv : mRtvs)
		{
			mRtvHeap.Free(rtv);
		}
	}

	// バッファのサイズを変更する
	void SwapChain::Resize(uint32_t width, uint32_t height)
	{
		if (width == mWidth && height == mHeight)
		{
			return;
		}

		// バックバッファへの参照が残っているとResizeBuffersが失敗する
		for (ComPtr<ID3D12Resource>& backBuffer : mBackBuffers)
		{
			backBuffer.Reset();
		}
		ThrowIfFailed(
			mSwapChain->ResizeBuffers(kBufferCount, width, height, kFormat, mFlags),
			"IDXGISwapChain::ResizeBuffers");
		mWidth = width;
		mHeight = height;

		CreateBackBuffers();
		Logger::Info("SwapChain resized: {}x{}", width, height);
	}

	// 表示する
	void SwapChain::Present(bool vsync)
	{
		const UINT syncInterval = vsync ? 1 : 0;
		// VSyncオフでティアリングを許可しないと、ボーダーレスでもリフレッシュレートで制限される
		const UINT flags = (!vsync && (mFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		const HRESULT hr = mSwapChain->Present(syncInterval, flags);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// TODO: デバイスを作り直して復帰する
			const HRESULT reason = mDevice->GetDeviceRemovedReason();
			ThrowIfFailed(FAILED(reason) ? reason : hr, "IDXGISwapChain::Present (device removed)");
		}
		ThrowIfFailed(hr, "IDXGISwapChain::Present");
	}

	// バックバッファを取得してRTVを作成
	void SwapChain::CreateBackBuffers()
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = kRtvFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (uint32_t i = 0; i < kBufferCount; ++i)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])), "IDXGISwapChain::GetBuffer");
			mBackBuffers[i]->SetName(std::format(L"BackBuffer{}", i).c_str());

			// リサイズ時はデスクリプタを使い回す
			if (!mRtvs[i].IsValid())
			{
				mRtvs[i] = mRtvHeap.Allocate();
			}
			mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), &rtvDesc, mRtvs[i].cpu);
		}
	}

} // namespace rd
