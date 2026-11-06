#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace rd
{

	// コマンドキューと完了待ち用のフェンス
	// Direct(描画)、Compute、Copy(非同期の転送)の種類ごとに作る
	// 複数のスレッドから呼び出せる
	class CommandQueue
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, const wchar_t* name);
		// 実行中のコマンドの完了を待ってから破棄する
		~CommandQueue();
		CommandQueue(const CommandQueue&) = delete;
		CommandQueue& operator=(const CommandQueue&) = delete;

		// 閉じたコマンドリストを実行し、完了を表すフェンス値を返す
		uint64_t Execute(ID3D12CommandList* commandList);
		// これまでに実行したコマンドの完了を表すフェンス値を返す
		uint64_t Signal();

		// フェンス値までのコマンドが完了したか
		bool IsFenceComplete(uint64_t fenceValue) const;
		// フェンス値までのコマンドの完了をCPUで待つ
		void WaitForFence(uint64_t fenceValue);
		// 別のキューのフェンス値までのコマンドが完了するまで、このキューの実行をGPU側で待たせる
		// (例: Copyキューで転送したテクスチャをDirectキューで使う前)
		void WaitForQueue(const CommandQueue& other, uint64_t fenceValue);
		// すべてのコマンドの完了を待つ
		void WaitIdle();

		// 再利用できるコマンドアロケータを取得(なければ作成)
		ID3D12CommandAllocator* RequestAllocator();
		// コマンドアロケータを返却する(fenceValueの完了後に再利用される)
		void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

		// アクセッサ
		ID3D12CommandQueue* GetQueue() const { return mQueue.Get(); }
		D3D12_COMMAND_LIST_TYPE GetType() const { return mType; }

	private:
		uint64_t SignalLocked();

	private:
		ID3D12Device* mDevice = nullptr;
		D3D12_COMMAND_LIST_TYPE mType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;

		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		// 次にSignalするフェンス値(フェンスの初期値0は完了済み扱い)
		uint64_t mNextFenceValue = 1;
		// 実行とSignalの順番を保つ
		std::mutex mFenceMutex;

		// 作成したすべてのアロケータ
		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mAllocators;
		// 返却されたアロケータと、再利用できるようになるフェンス値
		std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> mReadyAllocators;
		std::mutex mAllocatorMutex;
	};

} // namespace rd
