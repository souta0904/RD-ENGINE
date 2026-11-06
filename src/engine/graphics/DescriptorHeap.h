#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

namespace rd
{

	// デスクリプタヒープ内の1つのデスクリプタ
	struct DescriptorHandle
	{
		static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

		D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
		// シェーダから見えないヒープでは0
		D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
		// ヒープの先頭からの番号(Bindlessでシェーダに渡す番号にもなる)
		uint32_t index = kInvalidIndex;

		bool IsValid() const { return index != kInvalidIndex; }
	};

	// デスクリプタヒープ
	// 解放したデスクリプタは再利用する
	// 複数のスレッドから呼び出せる
	class DescriptorHeap
	{
	public:
		// 作成に失敗した場合はstd::runtime_errorを投げる
		DescriptorHeap(
			ID3D12Device* device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			uint32_t capacity,
			bool isShaderVisible,
			const wchar_t* name);
		DescriptorHeap(const DescriptorHeap&) = delete;
		DescriptorHeap& operator=(const DescriptorHeap&) = delete;

		// デスクリプタを割り当てる(空きがなければstd::runtime_errorを投げる)
		DescriptorHandle Allocate();
		// デスクリプタを解放して、handleを無効にする
		// TODO: GPUが使用中のデスクリプタを解放しないように、フェンス値を見て遅延解放する
		void Free(DescriptorHandle& handle);

		// アクセッサ
		ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }
		uint32_t GetCapacity() const { return mCapacity; }

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
		uint32_t mCapacity = 0;
		uint32_t mIncrementSize = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE mCpuStart = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mGpuStart = {};
		bool mIsShaderVisible = false;

		// まだ一度も割り当てていない先頭の番号
		uint32_t mNextIndex = 0;
		// 解放された番号
		std::vector<uint32_t> mFreeIndices;
		std::mutex mMutex;
	};

} // namespace rd
