#include "Utils.h"

#include <DirectX/d3dx12.h>

#include <sstream>
#include <comdef.h>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNr):
	m_ErrorCode(hr),
	m_FunctionName(functionName),
	m_FileName(fileName),
	m_LineNr(lineNr)
{
}

std::wstring DxException::ToString() const
{
	//Get the string description of the error code
	_com_error err(m_ErrorCode);
	std::wstring msg = err.ErrorMessage();

	std::wstringstream ss;

	ss << m_FunctionName << L" failed in " << m_FileName << L" on line " << m_LineNr << L"\n";
	ss << L"error: " << msg << L"\n";

	return ss.str();
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	//For static geometry we put vertex buffers in the default heap for optimal performance.
	//Generally, most geometry in a game will be like this.
	//After the vertex buffer has been initialized, only the GPU needs to read from the vertex buffer to draw the geometry,
	//so the default heap makes sense.
	//however, if the CPU cannot write to the vertex buffer in the default heap, how do we initialize the vertex buffer?

	//In addition to creating the actual vertex buffer resource, we need to create an intermediate upload buffer resource
	//with heap type D3D12_HEAP_TYPE_UPLOAD.
	//We need to commit a resource to the upload heap when we need to copy data from CPU to GPU.
	//After we create the upload buffer, we copy our vertex data from system memory to the upload buffer, 
	//and then we copy the vertex data from the upload buffer to the actual vertex buffer.
	using namespace Microsoft::WRL;

	ComPtr<ID3D12Resource> defaultBuffer;
	
	//Create the actual default buffer resource.
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	//In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap.
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	//Describe the data we want to copy into the default buffer
	//typedef struct D3D12_SUBRESOURCE_DATA
	//{
	//	const void* pData; // a pointer to a system memory array wich contains the data to initialize
	//the buffer with. If the buffer can store n vertices, then the system array must contain at least n vertices
	//so that the entire buffer can be initialized.
	//	LONG_PTR RowPitch; // For buffers, the size of the data we are copying in bytes.
	//  LONG_PTR SlicePitch; // For buffers, the size of the data we are copying in bytes.
	//} D3D12_SUBRESOURCE_DATA;
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	//Schedule to copy the data to the default buffer resource.
	//At a high level, the helper function UpdateSubresources will copy the CPU memory into
	//the intermediate upload heap.
	//Then, using ID3D12CommandList::CopySubresourceRegion, the intermediate upload heap data will be copied
	//to m_Buffer
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	//Note: uploadBuffer has to be kept alive after the above function calls
	//because the command list has not been executed yet that performs the actual copy.
	//The caller can release the uploadBuffer after it knows the copy has been executed.

	return defaultBuffer;
}