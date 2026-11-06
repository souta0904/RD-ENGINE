#include "GraphicsDevice.h"
#include "GraphicsUtility.h"
#include "Logger.h"
#include "StringUtility.h"
#include <iterator>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace rd
{

	using Microsoft::WRL::ComPtr;
	using GraphicsUtility::ThrowIfFailed;

	namespace
	{
		// 最低限必要な機能レベル
		constexpr D3D_FEATURE_LEVEL kMinFeatureLevel = D3D_FEATURE_LEVEL_12_0;

		const char* ToString(D3D_FEATURE_LEVEL level)
		{
			switch (level)
			{
			case D3D_FEATURE_LEVEL_12_2: return "12.2";
			case D3D_FEATURE_LEVEL_12_1: return "12.1";
			case D3D_FEATURE_LEVEL_12_0: return "12.0";
			default: return "unknown";
			}
		}

		// デバッグレイヤーのメッセージをLoggerへ出力
		void CALLBACK OnDebugMessage(
			D3D12_MESSAGE_CATEGORY,
			D3D12_MESSAGE_SEVERITY severity,
			D3D12_MESSAGE_ID id,
			LPCSTR description,
			void*)
		{
			switch (severity)
			{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			case D3D12_MESSAGE_SEVERITY_ERROR:
				Logger::Error("D3D12: {} (ID {})", description, static_cast<int>(id));
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				Logger::Warning("D3D12: {} (ID {})", description, static_cast<int>(id));
				break;
			default:
				Logger::Debug("D3D12: {} (ID {})", description, static_cast<int>(id));
				break;
			}
		}
	}

	GraphicsDevice::GraphicsDevice()
	{
		UINT factoryFlags = 0;
#ifdef _DEBUG
		if (EnableDebugLayer())
		{
			factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

		ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory)), "CreateDXGIFactory2");
		SelectAdapter();
		CreateDevice();
#ifdef _DEBUG
		SetupInfoQueue();
#endif
		CheckTearingSupport();

		Logger::Info("GraphicsDevice initialized");
	}

	GraphicsDevice::~GraphicsDevice()
	{
		if (mInfoQueue && mCallbackCookie != 0)
		{
			mInfoQueue->UnregisterMessageCallback(mCallbackCookie);
		}
		Logger::Info("GraphicsDevice destroyed");
	}

	// デバッグレイヤーを有効にする
	bool GraphicsDevice::EnableDebugLayer()
	{
		ComPtr<ID3D12Debug1> debug;
		if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			Logger::Warning("D3D12 debug layer is not available (Graphics Tools may not be installed)");
			return false;
		}
		debug->EnableDebugLayer();
		// GPUベースの検証は重いので必要なときだけ有効にする
		// debug->SetEnableGPUBasedValidation(TRUE);
		Logger::Info("D3D12 debug layer enabled");
		return true;
	}

	// ソフトウェアではない高性能なアダプタを選ぶ
	void GraphicsDevice::SelectAdapter()
	{
		for (UINT i = 0;; ++i)
		{
			ComPtr<IDXGIAdapter4> adapter;
			const HRESULT hr = mFactory->EnumAdapterByGpuPreference(
				i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}
			if (FAILED(hr))
			{
				continue;
			}

			DXGI_ADAPTER_DESC3 desc = {};
			if (FAILED(adapter->GetDesc3(&desc)) || (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
			{
				continue;
			}
			// デバイスを作成できるか確認だけする
			if (FAILED(D3D12CreateDevice(adapter.Get(), kMinFeatureLevel, __uuidof(ID3D12Device), nullptr)))
			{
				continue;
			}

			mAdapter = adapter;
			Logger::Info(
				"GPU: {} (VRAM {} MB)",
				StringUtility::ToUtf8(desc.Description),
				desc.DedicatedVideoMemory / (1024 * 1024));
			return;
		}

		Logger::Error("No GPU supports Direct3D 12 feature level {}", ToString(kMinFeatureLevel));
		throw std::runtime_error("No Direct3D 12 compatible GPU found");
	}

	void GraphicsDevice::CreateDevice()
	{
		ThrowIfFailed(D3D12CreateDevice(mAdapter.Get(), kMinFeatureLevel, IID_PPV_ARGS(&mDevice)), "D3D12CreateDevice");
		mDevice->SetName(L"Device");

		// 対応している最も高い機能レベルを調べる
		const D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
		};
		D3D12_FEATURE_DATA_FEATURE_LEVELS data = {};
		data.NumFeatureLevels = static_cast<UINT>(std::size(featureLevels));
		data.pFeatureLevelsRequested = featureLevels;
		if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &data, sizeof(data))))
		{
			mFeatureLevel = data.MaxSupportedFeatureLevel;
		}
		Logger::Info("D3D12 device created: feature level {}", ToString(mFeatureLevel));
	}

	// デバッグレイヤーのメッセージをLoggerへ流す
	void GraphicsDevice::SetupInfoQueue()
	{
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (FAILED(mDevice.As(&infoQueue)))
		{
			// デバッグレイヤーが無効
			return;
		}

		// デバッガで実行中はエラーで止める
		if (IsDebuggerPresent())
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		}

		// Windows 11のデバッグレイヤーの不具合で出るエラーは無視
		// https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_MESSAGE_ID ids[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumSeverities = static_cast<UINT>(std::size(severities));
		filter.DenyList.pSeverityList = severities;
		filter.DenyList.NumIDs = static_cast<UINT>(std::size(ids));
		filter.DenyList.pIDList = ids;
		infoQueue->PushStorageFilter(&filter);

		// コールバックはWindows 11以降のみ
		if (SUCCEEDED(infoQueue.As(&mInfoQueue)) &&
			SUCCEEDED(mInfoQueue->RegisterMessageCallback(
				OnDebugMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &mCallbackCookie)))
		{
			// Loggerも出力ウィンドウへ出すので二重に出さない
			infoQueue->SetMuteDebugOutput(TRUE);
			Logger::Info("D3D12 debug messages are routed to the log");
		}
		else
		{
			mInfoQueue.Reset();
			Logger::Warning("ID3D12InfoQueue1 is not available; debug messages go to the output window only");
		}
	}

	void GraphicsDevice::CheckTearingSupport()
	{
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(mFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			mIsTearingSupported = allowTearing == TRUE;
		}
		Logger::Info("Tearing supported: {}", mIsTearingSupported);
	}

} // namespace rd
