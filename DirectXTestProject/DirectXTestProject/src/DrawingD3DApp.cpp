#pragma region MyCode


#include "DrawingD3DApp.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <wrl.h>
#include <iostream>

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

	// Reset the command list to prep for initialization commands
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	this->BuildDescriptorHeaps();
	this->BuildConstantBuffers();
	this->BuildRootSignature();
	this->BuildShaderAndInputLayout();
	this->BuildBoxGeometry();
	this->BuildPSO();
	
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	this->FlushCommandQueue();

	return S_OK;
	// The root signature only defines what resources the application will bind to the rendering pipeline
	// it does not actually do any resource binding.
	// once a root signature has been set with a command list, we use the 
	// ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable to bind a descriptor table to the pipeline
	
	// RootParameterIndex: index of the root paramter we are setting
	// BaseDescriptor: Handle to a descriptor in the heap that specifies the first descriptor in the table
	// being set. For examplle, if the root signature specified that this table had five descriptors, then
	// BaseDescriptor and the next four descriptors in the heap are being set to this root table.
	// ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
	
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	ID3D12DescriptorHeap* descriptorHeaps[] = 
	{
		m_CbvHeap.Get()
	};

	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Offset the cbv we want to use for this draw call
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
	cbv.Offset(/*cbvIndex*/0, m_CbvSrvDescriptorSize);

	// Note: for performance, make the root signature as small as possible, and try to minimize the
	// number of times you change the root signature per rendering frame

	// The contents of the Root Signature (the descriptor tables, root constants and root descriptors)
	// that the application has bound automatically get versioned by the D3D12 driver whenever any part
	// of the contents change between draw/dispatch calls. So each draw/dispatch gets an unique full set of
	// Root Signature state.

	// If you change the root signature then you lose all the exisiting bindings.
	// That is, you need to rebind all the resources to the pipeline the new root signature expects.

	return S_OK;
}

void DrawingD3DApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*XM_PI, this->GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
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

	XMMATRIX world = XMLoadFloat4x4(&m_World);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);

	XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	m_ObjectConstantBuffer->CopyData(0, objConstants);
}

void DrawingD3DApp::Draw()
{
	// Reuse the memory assocaited with recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(m_DirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), m_Pso.Get()));

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorsRect);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), Colors::Blue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &this->GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	m_CommandList->IASetVertexBuffers(0, 1, &m_BoxGeo->GetVertexBufferView());
	m_CommandList->IASetIndexBuffer(&m_BoxGeo->GetIndexBufferView());
	m_CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //no Direct X 12 version

	m_CommandList->SetGraphicsRootDescriptorTable(0, m_CbvHeap->GetGPUDescriptorHandleForHeapStart());

	m_CommandList->DrawIndexedInstanced(m_BoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution
	ID3D12CommandList* cmdsLists[]{ m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(1, 0));

	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	// Wait until frame commands are complete. This waiting is inefficient and is done for simplicity.
	// Later we will show how to organize our rendering code so we do not have to wait per frame.
	this->FlushCommandQueue();
}

void DrawingD3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(m_WindowHandle);
}

void DrawingD3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
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

void DrawingD3DApp::BuildDescriptorHeaps()
{
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
}

void DrawingD3DApp::BuildConstantBuffers()
{
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
}

void DrawingD3DApp::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant buffers, textures, samplers).
	// the root signature defines the resources the shader programs expects.
	// If we think of a shader program as a functoin, and the input resources as function parameters,
	// then the root signature can be thought of as defining the function signature.

	// Root parameter can be a table root descriptor or root constants

	CD3DX12_ROOT_PARAMETER slotRootParamter[1];

	// Create a single descriptor table of CBVs
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, // number of descriptors in table
		0  // base shader register arguments are bound to for this root paramter
	);

	slotRootParamter[0].InitAsDescriptorTable
	(
		1, //number of ranges
		&cbvTable //pointer to array of ranges
	);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc
	(
		1,
		slotRootParamter,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// create a root signature with a single slot which points to a descriptor range
	// consisting of a single constant buffer.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
		std::cout << (char*)errorBlob->GetBufferPointer();

	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)
	));
}

void DrawingD3DApp::BuildShaderAndInputLayout()
{
	// In Direct3D, shader programs must first be compiled to a portable bytecode.
	// the graphics driver will then take this bytecode and compile it again into optimal
	// native instructions for the system's GPU. At runtime, we can compile a shader with the following instruction
	// HRESULT D3DCompileFromFile
	// (
	//	LPCWSTR pFileName, // The name of the .hlsl file that contains the HLSL source code we want to compile
	//	const D3D_SHADER_MACRO* pDefines, // Advanced option we do not use.
	//	ID3DInclude *pInclude, // Advanced option we do not use
	//  LPCSTR pEntrypoint, // the function name of the shader's entry point. A HLSL can contain multiple shader programs
	//  eg.(a vertex and pixel shadr), so we need to specify the entry point of the particular shader we want to compile.
	//  LPCSTR pTarget // A string specifying the shader program type and version we are using.
	//  1) vs_5_0 and vs_5_1: vertex shader 5.0 and 5.1
	//  2) hs_5_0 and hs_5_1: hull shader 5.0 and 5.1
	//  3) ds_5_0 and ds_5_1: domain shader 5.0 and 5.1
	//  4) gs_5_0 and gs_5_1: geometry shader 5.0 and 5.1
	//  5) ps_5_0 and ps_5_1: pixel shader 5.0 and 5.1
	//  6) cs_5_0 and cs_5_1: compute shader 5.0 and 5.1
	//	UINT Flags1, // Flags to specify how the shader code should be compiled. (D3DCOMPILE_DEBUG, D3DCOMPILE_SKIP_OPTIMIZATOIN)
	//	UINT Flags2, // Advanced effect compilatoin options we do not use. 
	//	ID3DBlob** ppCode, // Returns a pointer to a ID3DBlob data structure that stores the compiled shader object byte code
	//	ID3DBlob** ppErrorMsgs // Returns a pointer to a ID3DBlob data structure that stores a string containing the cimpilation errors
	// );

	// The type ID3DBlob is just a geneeric chunk of memory that has 2 methods:
	// LPVOID GetBufferPointer: Returns a void* to the data, so it must be casted to the appropriate type before use)
	// SIZE_T GetBufferSize: Returns the byte size of the buffer

	// m_VsByteCode = CompileShader(L"../../Data/Shaders/shader.fx", nullptr, "VS", "vs_5_0");
	// m_PsByteCode = CompileShader(L"Shader/shader.fx", nullptr, "PS", "ps_5_0");

	m_VsByteCode = LoadBinary(L"../Data/Shaders/bin/debug/shader_debug_vs.cso");
	m_PsByteCode = LoadBinary(L"../Data/Shaders/bin/debug/shader_debug_ps.cso");

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
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
}

void DrawingD3DApp::BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   }),
		Vertex({ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT4(Colors::Black)   }),
		Vertex({ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT4(Colors::Red)     }),
		Vertex({ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT4(Colors::Blue)    }),
		Vertex({ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT4(Colors::Yellow)  }),
		Vertex({ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT4(Colors::Cyan)    }),
		Vertex({ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT4(Colors::Magenta) })
	};

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

	const UINT64 vbByteSize = vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

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

	m_BoxGeo = std::make_unique<MeshGeometry>();
	m_BoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_BoxGeo->VertexBufferCPU));
	CopyMemory(m_BoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_BoxGeo->IndexBufferCPU));
	CopyMemory(m_BoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	m_BoxGeo->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, m_BoxGeo->VertexBufferUploader);
	m_BoxGeo->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, m_BoxGeo->IndexBufferUploader);

	m_BoxGeo->VertexByteStride = sizeof(Vertex);
	m_BoxGeo->VertexBufferByteSize = vbByteSize;
	m_BoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_BoxGeo->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	m_BoxGeo->DrawArgs["box"] = subMesh;

	// with indices, we must use ID3D12GraphicsCommandList::DrawIndexInstanced method instead of DrawInstanced.
	// IndexCountPerInstance: The number of indices to draw.
	// InstanceCount: Used for an advanced technique called instancing.
	// StartIndexLocation: Index to an element in the index buffer that marks the starting point from which to begin reading indices.s
	// BaseVertexLocation: An integer value to be added to the indices used in this draw call before the vertices are fetched.
	// StartInstanceLocation: Used for an advanced technique called instancing.
	// ID3D12GraphicsCommandList::DrawIndexedInstanced(UINT IndexCountPerInstnace, UINT InstanceCount, UINT StartIndexLoaction, INT BaseVertexLocation, UINT StartInstanceLocation);
}

void DrawingD3DApp::BuildPSO()
{
	// Most of the objects that control the state of the graphics pipeline are specified as an
	// aggregate called a pipeline state object (PSO), which is represented by the 
	// ID3D12PipelineState interface. To create a PSO, we first describe it by filling out 

	//typedef struct D3D12_SHADER_BYTECODE
	//{
	//	const BYTE *pShaderByteCode;
	//	SIZE_T BytecodeLength;
	//} D3D12_SHADER_BYTECODE;

	//typedef struct D3D12_INPUT_LAYOUT_DESC
	//{
	//	const D3D12_INPUT_ELEMENT_DESC *pInputelementDescs;
	//	UINT NumElements;
	//} D3D12_INPUT_LAYOUT_DESC;
	
	// a D3D12_GRAPHICS_PIPELINE_STATE_DESC instance:
	// typedef struct D3D12_GRAPHICS_PIPLEINE_STATE_DESC
	// {
	//	ID3D12RootSignature *pRootSignature;					// Pointer to the root signature to be bound with this PSO
	//																The root signature must be compatible with the shaders specified with this PSO.
	//
	//	D3D12_SHADER_BYTECODE VS;								// The vertex shader to bind. This is specified by the D3D12_SHADER_BYTECODE
	//																structure which is a pointer to the compiled bytecode data, and the size of the bytecode data in bytes.
	//
	//	D3D12_SHADER_BYTECODE PS;								// The pixel shader to bind 
	//	D3D12_SHADER_BYTECODE DS;								// The domain shader to bind
	//  D3D12_SHADER_BYTECODE HS;								// the hull shader to bind
	//  D3D12_SHADER_BYTECODE GS;								// the geometry shader to bind
	//	D3D12_STREAM_OUTPUT_DESC StreamOutput;					// Used for an advanced technique called stream-out. we jsut zero-out this field for now
	//  D3D12_BLEND_DESC BlendState;							// Specifies the blend state which configures blending.
	//	UINT SampleMask;										// Multisampling can take up to 32 samples. This 3-bit integer value is used to enable/disbale the samples.
	//																For example, if you turn of the 5th bit, then the 5th sample will not be taken. Of course disabling the 5th
	//																sample only has any consequence if you are actually using multisampling with at least 5 samples.
	//																If an application is using sinlge sampling, then only the first bit of this parameter matters.
	//																Generally the default of 0xffffffff is used, which does not disable any samples.
	//
	//	D3D12_RASTERIZER_DESC RasterizerState;					// Specifies the rasterization state which configures the rasterizer.
	//	D3D12_DEPTH_STENCIL_DESC DepthStencilState;				// Specifies the depth/stencils tate which configures the depth/stencil test.
	//	D3D12_INPUT_LAYOUT_DESC InputLayout;					// an input layout description which is simply an array of D3D12_INPUT_ELEMENT_DESC elements, and the number of elements in the array.
	//	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;	// specifies the primitive topology type
	//																D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0
	//																D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT		= 1
	//																D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE		= 2
	//																D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3
	//																D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH		= 4	
	//
	//  UINT NumRenderTargets;									// The number of render targets we are using simultaneously
	//	DXGI_FORMAT RTVFormats[8];								// the render target formats. this is an array to support writing to multiple render targets simultaneously
	//																this should match the settings of the render target we are using the PSO with.
	//
	//	DXGI_FORMAT DSVFormat;									// the format of the depth/stencil buffer. this should match the settings of the depth/stencil buffer we are using the PSO with.
	//  DXGI_SAMPLE_DESC SampleDesc;							// Desribes the multisample count and quality level. This should match the settings of the render target we are using.
	// } D3D12_GRAPHICS_PIPELINE_STATE_DESC;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(m_VsByteCode->GetBufferPointer()), m_VsByteCode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(m_PsByteCode->GetBufferPointer()), m_PsByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.SampleDesc.Count = m_4xMsaaEnabled ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_4xMsaaEnabled ? (m_4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = m_DepthStencilFormat;

	ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_Pso.GetAddressOf())));

	// This is quite a lot of state in one aggregate ID3D12PipelineState object. 
	// We specify all these objects as an aggregate to the graphics pipeline for performance.
	// By specifying them as an aggregate, Direct3D can validate that all the state is compatible and
	// the driver can generate all the code up front to program the hardware state.
	// In the Direct3D 11 state model, these render states pieces were set separately.
	// However, the states are related. If one piece of state gets changed, it may additionally require the driver
	// to reprogram the hardware for another piece of dependent state.As many states are
	// changed to configure the pipeline, the state of the hardware could get reprogrammed redundantly.
	// To avoid this redundancy, the drivers typically deferred programming the hardware state until 
	// a draw call is issued when the entire pipeline state would be known.
	// But this deferral requires additional bookkeeping work by the driver at runtime. 
	// It needs to track which states have changed, and then generate the code to program the hardware state at runtime.
	// In the new Direct3D 12 model, the driver can generate all the code needed to program the pipeline state at initialization 
	// time because we specify the majority of pipeline state as an aggregate.

	// Note: Becasue PSO validatoin and creation can be time consuming, PSOs should be generated at initialization time. 
	// once exception to this might be to create a PSO at runtime on demand the first time it is referenced.
	// then store it in a collectoin such as a hash table so it can quickly be fetched for future use.

	// Switching between PSO:
	// Reset specifies initial PSO.
	// m_CommandList->Reset(m_DirectCmdListAlloc.Get(), m_Pso1.Get());
	// Draw objects using PSO1
	//
	// Change PSO
	// m_CommandList->SetPipelineState(m_PSO2.Get());
	// Draw objects using PSO2
	//
	// Change PSO
	// m_CommandList->SetPipelineState(m_PSO.Get());
	// Draw Objects using PSO 3

	// Note: PSO state changes should be kept to a minimum for performance.
	// Draw all objects together that can use the same PSO. Do not change the PSO per draw call!
}



// typedef struct D3D12_RASTERIZER_DESC 
// {
//	D3D12_FILL_MODE Fillmode;	// Default: D3D12_FILL_SOLID
//	D3D12_CULL_MODE Cullmode;	// Default: D3D12_CULL_BACK
//	BOOL FrontCounterClockWise; // Default: false
//	INT DepthBias;				// Default: 0
//	FLOAT DepthBiasClamp;		// Default: 0.0f
//	FLOAT SlopeScaledDepthBias;	// Default: 0.0f
//	BOOL DepthClipEnable;		// Default: true
//  BOOL ScissorEnable;			// Default: false
//	BOOL MultisampleEnable;		// Default: false
//	BOOL AntialiasedLineEnable;	// Default: false
//	UINT ForcedSampleCount;		// Default: 0
//  D3D12_CONSERVATIE__RATERIZATION_MODE ConservativeRater;	// Default: D3D12_CONSERVATIE_RASTERIZATION_MODE_OFF
// };

// FillMode: Specify D3D12_FILL_WIREFRAME for wireframe rendering or D3D12_FILL_SOLID for solid rendering.
// CullMode: Specify D3D12_CULL_NONE to disbale culling.
// D3D12_CULL_BACK to cul back-facing trianggles, or D3D12_CULL_FRONT to cull front facing trianges.
// FrontCounterClockwise: Specify false if you want triangles ordered clockwise (with respect to the camera)
// to be treated as front-cacing and triangles ordered counterclockwise (with respect to the camera) to be treated
// as back-facing. specify true for the reverse.
//ScissorEnable: Specify true to enable the scissor test adn false to disable it.

// The following code shows how to create a rasterize state that turns on wireframe mode and siable backface culling.

// CD3DX12_RASTERIZER_dESC rsDesc(D#D12_DEFAULT);
// rsDesc.FillMode = D3D12_FILL_WIREFRAME;
// rsDesc.CullMode = D3D12_CULL_NONE;

// CD3D12_RASTERIZER_DESC is a convenience class that extends D3D12_RASTERIZER_DESC and adds some helper
// constructors. In particular, it has a constructor that takes an object of type CD3D12_DEFAULT, 
// which is just a dummy type used for overloading to indicate the rasterizer state members should be initialized to
// the default values. CD3D12_DEFAULT AND D3D12_DEFAULT are defined like so:
// struct CD3D12_DEFAULT {};
// externs const DECLSPEC_SELECTANY CD3D12_DEFAULT D3D12_DEFAULT;


#pragma endregion
