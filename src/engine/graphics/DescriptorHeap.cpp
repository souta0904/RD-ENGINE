#include "DescriptorHeap.h"
#include "GraphicsUtility.h"
#include "Logger.h"
#include "StringUtility.h"
#include <cassert>
#include <stdexcept>

namespace rd
{

	using GraphicsUtility::ThrowIfFailed;

	DescriptorHeap::DescriptorHeap(
		ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t capacity,
		bool isShaderVisible,
		const wchar_t* name)
		: mCapacity(capacity)
		, mIsShaderVisible(isShaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = type;
		desc.NumDescriptors = capacity;
		desc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)), "CreateDescriptorHeap");
		mHeap->SetName(name);

		mIncrementSize = device->GetDescriptorHandleIncrementSize(type);
		mCpuStart = mHeap->GetCPUDescriptorHandleForHeapStart();
		if (isShaderVisible)
		{
			mGpuStart = mHeap->GetGPUDescriptorHandleForHeapStart();
		}

		Logger::Info(
			"DescriptorHeap created: {} (capacity {}, shader visible {})",
			StringUtility::ToUtf8(name),
			capacity,
			isShaderVisible);
	}

	// デスクリプタを割り当てる
	DescriptorHandle DescriptorHeap::Allocate()
	{
		std::lock_guard lock(mMutex);

		uint32_t index = DescriptorHandle::kInvalidIndex;
		if (!mFreeIndices.empty())
		{
			index = mFreeIndices.back();
			mFreeIndices.pop_back();
		}
		else if (mNextIndex < mCapacity)
		{
			index = mNextIndex++;
		}
		else
		{
			Logger::Error("DescriptorHeap is full (capacity {})", mCapacity);
			throw std::runtime_error("DescriptorHeap is full");
		}

		DescriptorHandle handle;
		handle.index = index;
		handle.cpu.ptr = mCpuStart.ptr + static_cast<SIZE_T>(index) * mIncrementSize;
		if (mIsShaderVisible)
		{
			handle.gpu.ptr = mGpuStart.ptr + static_cast<UINT64>(index) * mIncrementSize;
		}
		return handle;
	}

	// デスクリプタを解放して、handleを無効にする
	void DescriptorHeap::Free(DescriptorHandle& handle)
	{
		if (!handle.IsValid())
		{
			return;
		}
		std::lock_guard lock(mMutex);
		assert(handle.index < mNextIndex && "DescriptorHandle does not belong to this heap");
		mFreeIndices.push_back(handle.index);
		handle = {};
	}

} // namespace rd
