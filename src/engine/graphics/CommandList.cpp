#include "CommandList.h"
#include "CommandQueue.h"
#include "GraphicsUtility.h"
#include <cassert>

namespace rd
{

	using GraphicsUtility::ThrowIfFailed;

	CommandList::CommandList(ID3D12Device* device, CommandQueue& queue, const wchar_t* name)
		: mQueue(queue)
	{
		// 作成直後は記録中なので、アロケータを借りてすぐに閉じる
		ID3D12CommandAllocator* allocator = queue.RequestAllocator();
		const HRESULT hr = device->CreateCommandList(
			0, queue.GetType(), allocator, nullptr, IID_PPV_ARGS(&mList));
		// 何も実行していないのですぐに再利用できる(フェンス値0は完了済み)
		queue.DiscardAllocator(0, allocator);
		ThrowIfFailed(hr, "CreateCommandList");

		mList->SetName(name);
		ThrowIfFailed(mList->Close(), "ID3D12GraphicsCommandList::Close");
	}

	CommandList::~CommandList()
	{
		// 記録したまま実行しなかった場合はアロケータを返す
		if (mAllocator)
		{
			mList->Close();
			mQueue.DiscardAllocator(0, mAllocator);
		}
	}

	// 記録を開始
	void CommandList::Begin()
	{
		assert(!IsRecording() && "CommandList::Begin called twice without Execute");
		mAllocator = mQueue.RequestAllocator();
		ThrowIfFailed(mList->Reset(mAllocator, nullptr), "ID3D12GraphicsCommandList::Reset");
	}

	// 記録を終了して実行し、完了を表すフェンス値を返す
	uint64_t CommandList::Execute()
	{
		assert(IsRecording() && "CommandList::Execute called without Begin");
		ThrowIfFailed(mList->Close(), "ID3D12GraphicsCommandList::Close");
		const uint64_t fenceValue = mQueue.Execute(mList.Get());
		// 実行が完了したら再利用される
		mQueue.DiscardAllocator(fenceValue, mAllocator);
		mAllocator = nullptr;
		return fenceValue;
	}

	// リソースの状態を遷移させる
	void CommandList::Transition(
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after,
		UINT subresource)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = subresource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		mList->ResourceBarrier(1, &barrier);
	}

} // namespace rd
