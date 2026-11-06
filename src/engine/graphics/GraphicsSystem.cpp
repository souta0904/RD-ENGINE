#include "GraphicsSystem.h"
#include "GraphicsUtility.h"
#include "Logger.h"
#include "Window.h"

namespace rd
{

	using GraphicsUtility::ThrowIfFailed;

	namespace
	{
		constexpr uint32_t kRtvCapacity = 64;
		constexpr uint32_t kDsvCapacity = 16;
		constexpr uint32_t kSrvCapacity = 4096;

		constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;
		constexpr float kClearColor[4] = { 0.1f, 0.2f, 0.4f, 1.0f };
	}

	GraphicsSystem::GraphicsSystem(const Window& window)
		: mWindow(window)
		, mDevice()
		, mDirectQueue(mDevice.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"DirectQueue")
		, mComputeQueue(mDevice.GetDevice(), D3D12_COMMAND_LIST_TYPE_COMPUTE, L"ComputeQueue")
		, mCopyQueue(mDevice.GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY, L"CopyQueue")
		, mRtvHeap(mDevice.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kRtvCapacity, false, L"RtvHeap")
		, mDsvHeap(mDevice.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDsvCapacity, false, L"DsvHeap")
		, mSrvHeap(mDevice.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kSrvCapacity, true, L"SrvHeap")
		, mSwapChain(mDevice, mDirectQueue, mRtvHeap, window.GetHWnd(), window.GetWidth(), window.GetHeight())
		, mCommandList(mDevice.GetDevice(), mDirectQueue, L"FrameCommandList")
	{
		mDsv = mDsvHeap.Allocate();
		CreateDepthBuffer();
		Logger::Info("GraphicsSystem initialized");
	}

	GraphicsSystem::~GraphicsSystem()
	{
		// 各キューもデストラクタで待つが、スワップチェインなどを先に破棄するのでここでも待つ
		try
		{
			WaitIdle();
		}
		catch (...)
		{
			// ログはThrowIfFailedで出力済み
		}
		mDsvHeap.Free(mDsv);
		Logger::Info("GraphicsSystem shutting down");
	}

	// フレームを開始する
	void GraphicsSystem::BeginFrame()
	{
		if (mWindow.GetWidth() != mSwapChain.GetWidth() || mWindow.GetHeight() != mSwapChain.GetHeight())
		{
			Resize(mWindow.GetWidth(), mWindow.GetHeight());
		}

		// このバックバッファを使った前回のフレームの完了を待つ
		mBackBufferIndex = mSwapChain.GetCurrentBackBufferIndex();
		mDirectQueue.WaitForFence(mFrameFenceValues[mBackBufferIndex]);

		mCommandList.Begin();
		ID3D12GraphicsCommandList* commandList = mCommandList.Get();

		// 表示からレンダーターゲットへ
		mCommandList.Transition(
			mSwapChain.GetBackBuffer(mBackBufferIndex),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// レンダーターゲットを設定してクリア
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv = mSwapChain.GetRtv(mBackBufferIndex);
		commandList->OMSetRenderTargets(1, &rtv, FALSE, &mDsv.cpu);
		commandList->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);
		commandList->ClearDepthStencilView(mDsv.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// ビューポートとシザー矩形
		D3D12_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(mSwapChain.GetWidth());
		viewport.Height = static_cast<float>(mSwapChain.GetHeight());
		viewport.MaxDepth = 1.0f;
		commandList->RSSetViewports(1, &viewport);
		const D3D12_RECT scissor = {
			0,
			0,
			static_cast<LONG>(mSwapChain.GetWidth()),
			static_cast<LONG>(mSwapChain.GetHeight()) };
		commandList->RSSetScissorRects(1, &scissor);

		ID3D12DescriptorHeap* heaps[] = { mSrvHeap.GetHeap() };
		commandList->SetDescriptorHeaps(1, heaps);
	}

	// フレームを終了して表示する
	void GraphicsSystem::EndFrame()
	{
		// レンダーターゲットから表示へ
		mCommandList.Transition(
			mSwapChain.GetBackBuffer(mBackBufferIndex),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);

		mFrameFenceValues[mBackBufferIndex] = mCommandList.Execute();
		mSwapChain.Present(mVSync);
	}

	// すべてのキューの完了を待つ
	void GraphicsSystem::WaitIdle()
	{
		mDirectQueue.WaitIdle();
		mComputeQueue.WaitIdle();
		mCopyQueue.WaitIdle();
	}

	// ウィンドウのサイズに合わせてバッファを作り直す
	void GraphicsSystem::Resize(uint32_t width, uint32_t height)
	{
		// バックバッファと深度バッファを使っているコマンドの完了を待つ
		mDirectQueue.WaitIdle();
		mSwapChain.Resize(width, height);
		CreateDepthBuffer();
	}

	void GraphicsSystem::CreateDepthBuffer()
	{
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = mSwapChain.GetWidth();
		desc.Height = mSwapChain.GetHeight();
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = kDepthFormat;
		desc.SampleDesc.Count = 1;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = kDepthFormat;
		clearValue.DepthStencil.Depth = 1.0f;

		mDepthBuffer.Reset();
		ThrowIfFailed(
			mDevice.GetDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue,
				IID_PPV_ARGS(&mDepthBuffer)),
			"CreateCommittedResource(DepthBuffer)");
		mDepthBuffer->SetName(L"DepthBuffer");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = kDepthFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		mDevice.GetDevice()->CreateDepthStencilView(mDepthBuffer.Get(), &dsvDesc, mDsv.cpu);
		Logger::Debug("DepthBuffer created: {}x{}", desc.Width, desc.Height);
	}

} // namespace rd
