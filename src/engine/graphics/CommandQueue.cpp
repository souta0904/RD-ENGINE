#include "CommandQueue.h"
#include "GraphicsUtility.h"
#include "Logger.h"
#include "StringUtility.h"
#include <format>
#include <string>

namespace rd
{

	using Microsoft::WRL::ComPtr;
	using GraphicsUtility::ThrowIfFailed;

	CommandQueue::CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, const wchar_t* name)
		: mDevice(device)
		, mType(type)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue)), "CreateCommandQueue");
		mQueue->SetName(name);

		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)), "CreateFence");
		mFence->SetName((std::wstring(name) + L"Fence").c_str());

		Logger::Info("CommandQueue created: {}", StringUtility::ToUtf8(name));
	}

	CommandQueue::~CommandQueue()
	{
		// 実行中のコマンドが使っているアロケータやリソースを破棄しないように待つ
		try
		{
			WaitIdle();
		}
		catch (...)
		{
			// ログはThrowIfFailedで出力済み
		}
	}

	// 閉じたコマンドリストを実行し、完了を表すフェンス値を返す
	uint64_t CommandQueue::Execute(ID3D12CommandList* commandList)
	{
		std::lock_guard lock(mFenceMutex);
		ID3D12CommandList* commandLists[] = { commandList };
		mQueue->ExecuteCommandLists(1, commandLists);
		return SignalLocked();
	}

	// これまでに実行したコマンドの完了を表すフェンス値を返す
	uint64_t CommandQueue::Signal()
	{
		std::lock_guard lock(mFenceMutex);
		return SignalLocked();
	}

	uint64_t CommandQueue::SignalLocked()
	{
		const uint64_t fenceValue = mNextFenceValue++;
		ThrowIfFailed(mQueue->Signal(mFence.Get(), fenceValue), "ID3D12CommandQueue::Signal");
		return fenceValue;
	}

	// フェンス値までのコマンドが完了したか
	bool CommandQueue::IsFenceComplete(uint64_t fenceValue) const
	{
		return fenceValue <= mFence->GetCompletedValue();
	}

	// フェンス値までのコマンドの完了をCPUで待つ
	void CommandQueue::WaitForFence(uint64_t fenceValue)
	{
		if (IsFenceComplete(fenceValue))
		{
			return;
		}
		// イベントにnullptrを渡すと完了するまで戻らない(複数のスレッドから同時に待てる)
		ThrowIfFailed(mFence->SetEventOnCompletion(fenceValue, nullptr), "ID3D12Fence::SetEventOnCompletion");
	}

	// 別のキューのフェンス値までのコマンドが完了するまで、このキューの実行をGPU側で待たせる
	void CommandQueue::WaitForQueue(const CommandQueue& other, uint64_t fenceValue)
	{
		ThrowIfFailed(mQueue->Wait(other.mFence.Get(), fenceValue), "ID3D12CommandQueue::Wait");
	}

	// すべてのコマンドの完了を待つ
	void CommandQueue::WaitIdle()
	{
		WaitForFence(Signal());
	}

	// 再利用できるコマンドアロケータを取得(なければ作成)
	ID3D12CommandAllocator* CommandQueue::RequestAllocator()
	{
		std::lock_guard lock(mAllocatorMutex);

		if (!mReadyAllocators.empty() && IsFenceComplete(mReadyAllocators.front().first))
		{
			ID3D12CommandAllocator* allocator = mReadyAllocators.front().second;
			mReadyAllocators.pop();
			ThrowIfFailed(allocator->Reset(), "ID3D12CommandAllocator::Reset");
			return allocator;
		}

		ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(mDevice->CreateCommandAllocator(mType, IID_PPV_ARGS(&allocator)), "CreateCommandAllocator");
		allocator->SetName(std::format(L"CommandAllocator{}", mAllocators.size()).c_str());
		mAllocators.push_back(allocator);
		Logger::Debug("CommandAllocator created ({} in pool)", mAllocators.size());
		return allocator.Get();
	}

	// コマンドアロケータを返却する
	void CommandQueue::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
	{
		std::lock_guard lock(mAllocatorMutex);
		mReadyAllocators.emplace(fenceValue, allocator);
	}

} // namespace rd
