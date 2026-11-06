#pragma once
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "GraphicsDevice.h"
#include "SwapChain.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace rd
{

	class Window;

	// DirectX 12の初期化とフレームの開始・終了
	class GraphicsSystem
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		explicit GraphicsSystem(const Window& window);
		// GPUの完了を待ってから破棄する
		~GraphicsSystem();
		GraphicsSystem(const GraphicsSystem&) = delete;
		GraphicsSystem& operator=(const GraphicsSystem&) = delete;

		// フレームを開始する(バックバッファをクリアしてレンダーターゲットに設定)
		// ウィンドウのサイズが変わっていたらバッファを作り直す
		// ウィンドウが最小化されている間は呼ばないこと
		void BeginFrame();
		// フレームを終了して表示する
		void EndFrame();
		// すべてのキューの完了を待つ
		void WaitIdle();

		void SetVSync(bool vsync) { mVSync = vsync; }

		// アクセッサ
		ID3D12Device* GetDevice() const { return mDevice.GetDevice(); }
		CommandQueue& GetDirectQueue() { return mDirectQueue; }
		CommandQueue& GetComputeQueue() { return mComputeQueue; }
		// 非同期のアップロード(テクスチャの読み込みなど)に使う
		CommandQueue& GetCopyQueue() { return mCopyQueue; }
		// フレームの描画に使うコマンドリスト(BeginFrameからEndFrameの間のみ記録中)
		CommandList& GetCommandList() { return mCommandList; }
		DescriptorHeap& GetRtvHeap() { return mRtvHeap; }
		DescriptorHeap& GetDsvHeap() { return mDsvHeap; }
		DescriptorHeap& GetSrvHeap() { return mSrvHeap; }

	private:
		// ウィンドウのサイズに合わせてバッファを作り直す
		void Resize(uint32_t width, uint32_t height);
		void CreateDepthBuffer();

	private:
		const Window& mWindow;

		// 初期化の順番に並べている(破棄は逆順)
		GraphicsDevice mDevice;
		CommandQueue mDirectQueue;
		CommandQueue mComputeQueue;
		CommandQueue mCopyQueue;
		DescriptorHeap mRtvHeap;
		DescriptorHeap mDsvHeap;
		// シェーダから見えるCBV/SRV/UAV用
		DescriptorHeap mSrvHeap;
		SwapChain mSwapChain;
		CommandList mCommandList;

		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthBuffer;
		DescriptorHandle mDsv;

		// バックバッファごとに、最後に使ったフレームの完了を表すフェンス値
		uint64_t mFrameFenceValues[SwapChain::kBufferCount] = {};
		uint32_t mBackBufferIndex = 0;
		bool mVSync = true;
	};

} // namespace rd
