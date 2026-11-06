#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace rd
{

	class CommandQueue;

	// コマンドリスト
	// アロケータは記録を開始するたびにCommandQueueから借りるので、実行後すぐに次の記録を始められる
	// 1つのCommandListは同時に1つのスレッドからしか使えない
	// (非同期のテクスチャ読み込みなどでは、スレッドごとにCopyキュー用のCommandListを作る)
	class CommandList
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		CommandList(ID3D12Device* device, CommandQueue& queue, const wchar_t* name);
		~CommandList();
		CommandList(const CommandList&) = delete;
		CommandList& operator=(const CommandList&) = delete;

		// 記録を開始
		void Begin();
		// 記録を終了して実行し、完了を表すフェンス値を返す
		uint64_t Execute();

		// リソースの状態を遷移させる
		void Transition(
			ID3D12Resource* resource,
			D3D12_RESOURCE_STATES before,
			D3D12_RESOURCE_STATES after,
			UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

		// アクセッサ
		ID3D12GraphicsCommandList* Get() const { return mList.Get(); }
		CommandQueue& GetQueue() const { return mQueue; }
		bool IsRecording() const { return mAllocator != nullptr; }

	private:
		CommandQueue& mQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;
		// 記録中に使っているアロケータ(記録中でなければnullptr)
		ID3D12CommandAllocator* mAllocator = nullptr;
	};

} // namespace rd
