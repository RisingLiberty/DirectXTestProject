#pragma once

#include <d3d12.h>
#include <DirectX/d3dx12.h>
#include <wrl.h>
#include <memory.h>
#include "Utils.h"

// Typically, the world matrix of an object will change when it moves/rotates/scales,
// the view matrix changes when the camera moves/rotates.
// the projection matrix changes when the window is resized.

template <typename T>
class UploadBuffer
{
public: 
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		m_IsConstantBuffer(isConstantBuffer)
	{
		m_ElementByteSize = sizeof(T);

		// Constant buffer elements need to be multiples of 256 bytes
		// This is because the hardware can only view constant data
		// at m*256 byte offsets and of n*256 byte lenghts
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC
		// {
		//	UINT64 OffsetInBytes; // multiple of 256
		//	UINT SizeInByes; // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			m_ElementByteSize = CalculateConstantBufferByteSize(sizeof(T));

		// We can think of uploadBuffer as storing an array of constant buffers of type ObjectConstans(with padding to make a multiple of 256 bytes).
		// when it comes time to draw an object, we just bind a constant buffer view (CBV) to a subregion of the buffer
		// that stores the constants for that object. note that we will often call the buffer uploadBuffer a constant buffer
		// since it stores an array of constant buffers.
		ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadBuffer)));

		// the first parameter is a subresource index identifying the subresource to map.
		// for a buffer, the only subresource is the buffer itself, so we just set this to 0
		// the second parameter is an optional pointer to a D3D12_RANGE structure that describes the range of
		// memory to map. specifying null maps the entire resource.
		// the third parameter returns a pointer to the mapped data. 
		ThrowIfFailed(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(m_MappedData)));

		// We do not need to unmap until we are done with the resource.
		// However, we must not write to the resource while it is in use by the GPU(synchronize!).
	}

	UploadBuffer(const UploadBuffer& other) = delete;
	UploadBuffer& operator=(const UploadBuffer& other) = delete;
	~UploadBuffer()
	{
		// the first parameter to Unmap is a subresource index identifying the subresource to map,
		// which will be 0 for a buffer.
		// the second parameter to Unmap is an optional pointer to a D3D12_RANGE structure that describes
		// the range of memory to unmap. specifying null unmaps the entire resource.
		if (m_UploadBuffer)
			m_UploadBuffer->Unmap(0, nullptr);

		m_MappedData = nullptr;
	}

	ID3D12Resource* GetResource() const
	{
		return m_UploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		// to copy data from system memory to the buffer, we can just do a mem copy
		memcpy(&m_MappedData[elementIndex*m_ElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
	BYTE* m_MappedData = nullptr;
	UINT m_ElementByteSize = 0;
	bool m_IsConstantBuffer;
};