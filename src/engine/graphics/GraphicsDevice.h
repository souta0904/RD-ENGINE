#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace rd
{

	// DXGIファクトリー、アダプタ(GPU)、D3D12デバイス
	class GraphicsDevice
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		GraphicsDevice();
		~GraphicsDevice();
		GraphicsDevice(const GraphicsDevice&) = delete;
		GraphicsDevice& operator=(const GraphicsDevice&) = delete;

		// アクセッサ
		ID3D12Device* GetDevice() const { return mDevice.Get(); }
		IDXGIFactory6* GetFactory() const { return mFactory.Get(); }
		D3D_FEATURE_LEVEL GetFeatureLevel() const { return mFeatureLevel; }
		// VSyncオフ時にティアリングを許可できるか(可変リフレッシュレートのモニタ向け)
		bool IsTearingSupported() const { return mIsTearingSupported; }

	private:
		// デバッグレイヤーを有効にする(Graphics Toolsが入っていなければfalse)
		bool EnableDebugLayer();
		// ソフトウェアではない高性能なアダプタを選ぶ
		void SelectAdapter();
		void CreateDevice();
		// デバッグレイヤーのメッセージをLoggerへ流す
		void SetupInfoQueue();
		void CheckTearingSupport();

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory6> mFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
		D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_0;
		bool mIsTearingSupported = false;
		// デバッグレイヤーのメッセージのコールバック
		Microsoft::WRL::ComPtr<ID3D12InfoQueue1> mInfoQueue;
		DWORD mCallbackCookie = 0;
	};

} // namespace rd
