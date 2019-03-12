#include "DrawingD3DApp.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <wrl.h>

#include "Utils.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

DrawingD3DApp::DrawingD3DApp(HINSTANCE hInstance):
	D3DApp(hInstance)
{
	m_AppName = L"Drawing D3D App";
}

HRESULT DrawingD3DApp::Initialize()
{
	ThrowIfFailed(D3DApp::Initialize());

	// Input Layout
	// SemanticName: A string to associate with the element. This can be any valid variable name.
	// Semantics are used to map elements in the vertex strucutre to elements in the vertex shader input signature.
	// SemanticIndex: an index to attach to a semantic. A vertex strcuture may have more than one set of texture
	// coordinates, so rather than introducing a new semantic name, we can just attach an index to the end to distinguish
	// the two texture coordinate sets. A semantic with no index specified in the shader code defaults to index zero.
	// For instance: POSITION is equivalent to POSITION0
	// Format: A member of DXGI_FORMAT enumerated type specifying the format of this vertex element to Direct3D.
	// Common examples of formats:
	// 	DXGI_FORMAT_R32_FLOAT			1D 32-bit float
	// 	DXGI_FORMAT_R32G32_FLOAT		2d 32-bit float
	// 	DXGI_FORMAT_R32G32B32_FLOAT		3d 32-bit float
	// 	DXGI_FORMAT_R32G32B32A32_FLOAT	4d 32-bit float
	//  DXGI_FORMAT_R8_UINT				1d 8-bit unsigned int
	// 	DXGI_FORMAT_R16G16_SINT			2d 16-bit signed int
	// 	DXGI_FORMAT_R32G32B32_UINT		3d 32-bit unsigned int
	//  DXGI_FORMAT_R8G8B8A8_SINT		4d 8-bit signed int
	// 	DXGI_FORMAT_R8G8B8A8_UINT		4d 8-bit unsigned int
	// Input slot: Specifies the input slot index the element will come from.
	// Direct3D supports 16 input slots (indexed from 0 -15) through which you can feed vertex data.
	// AlignedByteOffset: The offset, in bytes, from the start of the C++ vertex structure of the specified
	// input slot to start of the vertex component. 
	// For example, in the following vertex strucutre, the element Pos has a 0-byte offset, since its start coincides
	// with the start of the vertex structure. The element Normal has a 12-byte offset because we have to skip over
	// the bytes of Posto get to the start of Normal. The element Tex0 has 24-byte offset because we need to skip
	// over the bytes of Pos and Normal to get to the start of Tex0. the element Tex1 has a 32-byte offset because
	// we need to skip over the bytes of Pos, normal and Tex0 to get tot he start of Tex1
	// struct Vertex2
	// {
	// 	XMFLOAT3 Pos;		// 0-byte offset
	// 	XMFLOAT3 Normal;	// 12-byte offset
	//  XMFLOAT2 Tex0;		// 24-byte offset
	//  XMFLOAT2 Tex1;		// 32-byte offset
	// };
	// InputSlotClass: specify D3D12_INPUT_PER_VERTEX_DATA.
	// InstanceDataStepRate: Specify 0

	m_InputLayout =
	{ {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	} };

	// width: refers to the number of bytes in the buffer.
	// CD3DX12_RESOURCE_DESC::buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0)

	// Note: CD3DX12_RESOURCE_DESC class also provides convenience methods for constructing a D3D12_RESOURCE_DESC 
	// that describes texture resources and querying information about the resource:

	// CD3DX12_RESOURCE_DESC::Tex1D
	// CD3DX12_RESOURCE_DESC::Tex2D
	// CD3DX12_RESOURCE_DESC::Tex3D

	// Note: In contrast with DirectX 11, all resources are represented by the ID3D12Resource interface,
	// and their type is specified in the Dimension field of the struct, where as in DirectX 11, you had
	// D3D11Buffer and D3D11Texture2D to specify the type of resource.

	// Box Geometry
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	const UINT64 vbByteSize = 8 * sizeof(Vertex);

	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;

	vertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, VertexBufferUploader);

	// In order to bind a vertex buffer to the pipeline, we need to create a vertex buffer view
	// to the vertex buffer resource. Unlike an RTV(render target view), we do not need a descriptor
	// heap for a vertex buffer view. A vertex buffer view is represented by the D3D12_VERTEX_BUFFER_VIEW_DESC strucutre:
	// typedef struct D3D12_VERTEX_BUFFER_VIEW
	// {
	// 	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation; // The virtual address of the vertex buffer resource
	// 	we want to create a view to. We can use the ID3D12Resrouce::GetGPUVirtualAddress method to get this
	// 	UINT SizeInBytes; The number of bytes to view in the vertex buffer starting from BufferLocation.
	//  UINT StrideInByes; The size of each vertex element, in bytes.
	// } D3D12_VERTEX_BUFFER_VIEW;

	// After a vertex buffer has been created and we have created a view to it, we can bind it
	// to an input slot of the pipeline to feed the vertices to the input assembler stage of the pipeline.
	// StartSlot: the input slot to start binding vertex buffers to. There are 16 input slots idexed from 0-15.
	// NumBuffers: the number of vertex buffers we are binding to the input slots. If the start slot
	// has index k and we are binding n buffers, then we are binding buffers to input slots Ik, Ik+1, ..., Ik+n-1.
	// pViews: Pointer to the first element of an array of vertex buffer views.
	// void ID312GraphicsCommandList::IASetVertexBuffers(UINT StartSlots, UINT NumBuffers, const D3D12_VERTEX_BUFFER_VIEW* pViews);

	/*
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(Vertex);
	vbv.SizeInBytes = 8 * sizeof(Vertex);

	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = { vbv };
	m_CommandList->IASetVertexBuffers(0, 1, vertexBuffers);

	// An vertex buffer will stay bound to an input slot until you change it.
	ID3D12Resource* Vb1; // stores vertices of type Vertex1.
	ID3D12Resource* Vb2; // stores vertices of type Vertex2.

	D3D12_VERTEX_BUFFER_VIEW VBView1;
	D3D12_VERTEX_BUFFER_VIEW VbView2;

	// Create the vertex buffers and views

	m_CommandList->IASetVertexBuffers(0, 1, &VBView1);
	// Draw objects using vertex buffer 1

	m_CommandList->IASetVertexBuffers(0, 1, &VbView2);
	// Draw objects using vertex buffer 2
	*/

	// Settings a vertex buffer to an input slot does not draw them, 
	// It only makes the vertices ready to be fed into the pipline.
	// The final step to actually draw the vertices is done with the ID3D12GraphicsCommandList::DrawInstance method.
	// VertexCountPerInstance: the number of vertices to draw.
	// InstanceCount: USed for an advanced technique called instancing.
	// StartVertexLocation: specifies the index(zero-based) of the first vertex in the vertex buffer to begin drawing.
	// StartInstanceLocation: USed for an advanced technique called instancing.
	// void ID3D12CommandList::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);

	// typedef struct D3D12_INDEX_BUFFER_VIEW
	// {
	// 	D3D12_GPU_VIRTUAL_ADDRES BufferLocation; // The virtual address of the index buffer resource we want
	// to create a view to. We can use the ID3D12Resource::GEtGPUVirtualAddress method to get this.
	// 	UINT SizeInBytes; // the number of bytes to view in the index buffer starting from buffer location
	// 	DXGI_FORMAT Format; // the format of the indices which must be either DXGI_FORMAT_R16_UINT for
	// 16-bit indices or DXGI_FORMAT_R32_UINT for 32-bit indices. You should use 16-bit indices to recude memory
	// and bandwith, and only use 32-bit indices if you have index values that need the extra 32-bit range.

	// As with vertex buffers, and other Direct3D resources for that matter, before we can use it, 
	// we need to bind it to the pipeline

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT ibByteSize = indices.size() * sizeof(indices.front());

	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	indexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, indexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;

	m_CommandList->IASetIndexBuffer(&ibv);

	// with indices, we must use ID3D12GraphicsCommandList::DrawIndexInstanced method instead of DrawInstanced.
	// IndexCountPerInstance: The number of indices to draw.
	// InstanceCount: Used for an advanced technique called instancing.
	// StartIndexLocation: Index to an element in the index buffer that marks the starting point from which to begin reading indices.s
	// BaseVertexLocation: An integer value to be added to the indices used in this draw call before the vertices are fetched.
	// StartInstanceLocation: Used for an advanced technique called instancing.
	// ID3D12GraphicsCommandList::DrawIndexedInstanced(UINT IndexCountPerInstnace, UINT InstanceCount, UINT StartIndexLoaction, INT BaseVertexLocation, UINT StartInstanceLocation);

	// Constant buffer descriptors live in a descriptor heap of type D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV.
	// Such a hepa can store a mixture of constant buffer, shader resource and unordered access descriptors.
	// to store these new types of descriptors we will need to create a new descriptor heap of this type:
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	m_pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_CbvHeap.ReleaseAndGetAddressOf()));

	// The above code is similar to how we created the render target and depth.stencil buffer descriptor heaps.
	// However, one important difference is that we specify the D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE flag
	// to indicate that these descriptors will be accessed by shader programs

	// A constant buffer view is created by filling out a D3D12_CONSTANT_BUFFER_VIEW_DESC instance and calling
	// ID3D12Device::CreateConstantBufferView.
	
	// n == 1
	m_ObjectConstantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(m_pDevice.Get(), 1, true);
	
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

	// Address to start of the buffer (0th constant buffer)
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_ObjectConstantBuffer->GetResource()->GetGPUVirtualAddress();

	//Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	// The D3D12_CONSTANT_BUFFER_VIEW_DESC strucutre describes a subset of the constant buffer
	// resource to bind to the HLSL constant buffer structure. As Mentioned, typically a constant
	// buffer stores an array of per-object constants for n objects, but we can get a view to the ith
	// object constant data by using the BufferLocation and SizeInByes.
	// The D3D12_CONSTANT_BUFFER_VIEW_DESC::SizeInBytes and D3D12_CONSTANT_BUFFER_VIEW_DESC::OffsetInBytes
	// members must be a multiple of 256 bytes due to hardware requirements.
	// for example, if you would've specified 64, the following error would occur.
	
	// D3D12 ERROR: ID3D12Device::CreateConstantBufferView:
	// SizeInBytes of 64 is invalid. Device requires SizeInBytes be a multiple of 256.

	// D3D12 ERROR : ID3D12Device::CreateConstantBufferView :
	// OffsetInBytes of 64 is invalid.Device requires OffsetInBytes be a multiple of 256.
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = objCBByteSize;

	m_pDevice->CreateConstantBufferView(&cbvDesc, m_CbvHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

void DrawingD3DApp::Update(float dTime)
{
	//Convert spherical to Cartesian coordinates
	float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Radius * cosf(m_Phi);

	//Build the view matrix
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_View, view);

	XMMATRIX world = XMLoadFloat4x4(m_World);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);

	XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	m_ObjectConstantBuffer->CopyData(0, objConstants);
}

void DrawingD3DApp::Draw()
{

}

void DrawingD3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{

}

void DrawingD3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{

}

void DrawingD3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (btnState & MK_LBUTTON)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

		//Update angles based on input to orbit camera around box.
		m_Theta += dx;
		m_Phi += dy;

		// Restrict the angle m_Phi
		m_Phi = Clamp(m_Phi, 0.1f, XM_PI - 0.1f);
	}
	else if (btnState & MK_RBUTTON)
	{
		// Make each pixel correspond to 0.00f unit in the scene.
		float dx = 0.005f * static_cast<float>(x - m_LastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_LastMousePos.y);

		// Update the camera radius based on input
		m_Radius += dx - dy;

		// Restrict the radius
		m_Radius = Clamp(m_Radius, 3.0f, 15.0f);
	}

	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}
