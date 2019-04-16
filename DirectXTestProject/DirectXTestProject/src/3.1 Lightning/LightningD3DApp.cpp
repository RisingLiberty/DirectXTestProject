//***************************************************************************************
// LitColumnsApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "1.0 Core//d3dApp.h"
#include "1.0 Core//GeometryGenerator.h"
#include "1.0 Core/FrameResource.h"
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <iostream>
#include <fstream>
#include <d3dcompiler.h>

#include "1.0 Core/D3DAppBase.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

//const int gNumFrameResources = 3;

//Lightweight structure stores parameters to draw a shape.  This will
//vary from app-to-app.

//class LightningD3DApp : public D3DAppBase
//{
//public:
//	LightningD3DApp(HINSTANCE hInstance);
//	LightningD3DApp(const LightningD3DApp& rhs) = delete;
//	LightningD3DApp& operator=(const LightningD3DApp& rhs) = delete;
//	~LightningD3DApp();
//
//	virtual HRESULT Initialize()override;
//
//private:
//	virtual void OnResize()override;
//	virtual void Update(const float dTime)override;
//	virtual void Draw()override;
//
//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//	void OnKeyboardInput(const GameTimer& gt);
//	void UpdateCamera(const GameTimer& gt);
//	void AnimateMaterials(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMaterialCBs(const GameTimer& gt);
//	void UpdateMainPassCB(const GameTimer& gt);
//
//	void BuildRootSignature();
//	void BuildShadersAndInputLayout();
//	void BuildShapeGeometry();
//	void BuildSkullGeometry();
//	void BuildPSOs();
//	void BuildFrameResources();
//	void BuildMaterials();
//	void BuildRenderItems();
//	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
//
//private:
//
//	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
//	FrameResource* m_CurrentFrameResource = nullptr;
//	int m_CurrentFrameResourceIndex = 0;
//
//	UINT m_CbvSrvDescriptorSize = 0;
//
//	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
//
//	ComPtr<ID3D12DescriptorHeap> m_SrvDescriptorHeap = nullptr;
//
//	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
//	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
//	//std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
//	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
//
//	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
//
//	ComPtr<ID3D12PipelineState> m_OpaquePSO = nullptr;
//
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> m_RenderItems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> m_OpaqueRenderItems;
//
//	PassConstants m_MainPassCB;
//
//	XMFLOAT3 m_EyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 m_View = MAT_4_IDENTITY;
//	XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;
//
//	float m_Theta = 1.5f*XM_PI;
//	float m_Phi = 0.2f*XM_PI;
//	float m_Radius = 15.0f;
//
//	POINT m_LastMousePos;
//};

#include "LightningD3DApp.h"

LightningD3DApp::LightningD3DApp(HINSTANCE hInstance)
	: D3DAppBase(hInstance)
{
}

HRESULT LightningD3DApp::Initialize()
{
	ThrowIfFailed(D3DApp::Initialize())

		// Reset the command list to prep for initialization commands.
		ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	m_CbvSrvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPsos();

	// Execute the initialization commands.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void LightningD3DApp::Update(const float dTime)
{
	D3DAppBase::Update(dTime);

	UpdateMaterialCBs(m_GameTimer);
}

void LightningD3DApp::Draw()
{
	ComPtr<ID3D12CommandAllocator> cmdListAlloc = m_CurrentFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList
	// Reusing the command list reuses memory
	ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_OpaquePSO.Get()));

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear the back buffer and depth buffer
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);

	// Specify the buffers we are going to render to
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &this->GetDepthStencilView());

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	ID3D12Resource* passCB = m_CurrentFrameResource->PassCB->GetResource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	this->DrawRenderItems(m_CommandList.Get(), m_OpaqueRenderItems);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// done recording commands
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution
	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(1, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point
	m_CurrentFrameResource->Fence = ++m_CurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set
	// until the GPU finishes processing all the commands prior to this Signal()
	m_pCommandQueue->Signal(m_pFence.Get(), m_CurrentFence);
}

void LightningD3DApp::UpdateMaterialCBs(const GameTimer& gt)
{
	UploadBuffer<MaterialConstants>* currentMaterialCB = m_CurrentFrameResource->MaterialCB.get();

	for (auto& material : m_Materials)
	{
		// Only update the cbuffer data if the constants have changed.
		// If the cbuffer data changes, it needs to be updated for each FrameResource
		Material* mat = material.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;

			currentMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void LightningD3DApp::BuildRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Create root CBV
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsConstantBufferView(2);

	// A root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		std::cout << (char*)errorBlob->GetBufferPointer();

	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf())));

}

void LightningD3DApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = CompileShader(L"../data/shaders/src/shader_lightning.fx", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = CompileShader(L"../data/shaders/src/shader_lightning.fx", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{ {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	} };
}

void LightningD3DApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;

	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	// We are concatenating all the geometry into one big vertex/index buffer.
	// so define the regions in the buffer each submesh covers.

	// cache the vertex offsets to each object in concatenated vertex buffer'
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	// Define the submesh geometry that cover different regions of the vertex index buffers.

	SubMeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubMeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubMeshGeometry sphereSubMesh;
	sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubMesh.StartIndexLocation = sphereIndexOffset;
	sphereSubMesh.BaseVertexLocation = sphereVertexOffset;

	SubMeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	// Extract the vertex elements we are interested in and pach the vertices
	// of all the meshes into one vertex buffer.

	size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();

	std::vector<LightningVertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
	}

	std::vector<uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightningVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	memcpy(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	memcpy(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);
	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(LightningVertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	geometry->DrawArgs["box"] = boxSubmesh;
	geometry->DrawArgs["grid"] = gridSubmesh;
	geometry->DrawArgs["sphere"] = sphereSubMesh;
	geometry->DrawArgs["cylinder"] = cylinderSubmesh;


	m_Geometries[geometry->Name] = std::move(geometry);
}

void LightningD3DApp::BuildSkullGeometry()
{
	std::ifstream fin("../data/Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<LightningVertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightningVertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(),
		m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(LightningVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["skull"] = submesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void LightningD3DApp::BuildPsos()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	opaquePsoDesc.pRootSignature = m_RootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m_4xMsaaEnabled ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m_4xMsaaEnabled ? (m_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = m_DepthStencilFormat;
	ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_OpaquePSO)));
}

void LightningD3DApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pDevice.Get(),
			1, (UINT)m_RenderItems.size(), (UINT)m_Materials.size()));
	}
}

void LightningD3DApp::BuildMaterials()
{
	using MaterialPtr = std::unique_ptr<Material>;
	MaterialPtr brick0 = std::make_unique<Material>();
	brick0->Name = "bricks0";
	brick0->MatCBIndex = 0;
	brick0->DiffuseSrvHeapIndex = 0;
	brick0->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
	brick0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	brick0->Roughness = 0.1f;

	MaterialPtr stone0 = std::make_unique<Material>();
	stone0->Name = "stone0";
	stone0->MatCBIndex = 1;
	stone0->DiffuseSrvHeapIndex = 1;
	stone0->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	MaterialPtr tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	tile0->MatCBIndex = 2;
	tile0->DiffuseSrvHeapIndex = 2;
	tile0->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;

	MaterialPtr skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 3;
	skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.04f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;

	m_Materials["bricks0"] = std::move(brick0);
	m_Materials["stone0"] = std::move(stone0);
	m_Materials["tile0"] = std::move(tile0);
	m_Materials["skullMat"] = std::move(skullMat);
}

void LightningD3DApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> boxRenderItem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRenderItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	XMStoreFloat4x4(&boxRenderItem->TexTransform, XMMatrixScaling(0.1f, 0.1f, 0.1f));
	boxRenderItem->ObjCBIndex = 0;
	boxRenderItem->Material = m_Materials["stone0"].get();
	boxRenderItem->Geometry = m_Geometries["shapeGeo"].get();
	boxRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRenderItem->IndexCount = boxRenderItem->Geometry->DrawArgs["box"].IndexCount;
	boxRenderItem->StartIndexLocation = boxRenderItem->Geometry->DrawArgs["box"].StartIndexLocation;
	boxRenderItem->BaseVertexLocation = boxRenderItem->Geometry->DrawArgs["box"].BaseVertexLocation;

	m_RenderItems.push_back(std::move(boxRenderItem));

	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();

	gridRenderItem->World = MAT_4_IDENTITY;
	gridRenderItem->ObjCBIndex = 1;
	gridRenderItem->Material = m_Materials["tile0"].get();
	gridRenderItem->Geometry = m_Geometries["shapeGeo"].get();
	gridRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_RenderItems.push_back(std::move(gridRenderItem));

	auto skullRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MAT_4_IDENTITY;
	skullRitem->ObjCBIndex = 2;
	skullRitem->Material = m_Materials["skullMat"].get();
	skullRitem->Geometry = m_Geometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geometry->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geometry->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geometry->DrawArgs["skull"].BaseVertexLocation;
	m_RenderItems.push_back(std::move(skullRitem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	UINT objCBIndex = 3;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Material = m_Materials["bricks0"].get();
		leftCylRitem->Geometry = m_Geometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Material = m_Materials["bricks0"].get();
		rightCylRitem->Geometry = m_Geometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = MAT_4_IDENTITY;
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Material = m_Materials["stone0"].get();
		leftSphereRitem->Geometry = m_Geometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = MAT_4_IDENTITY;
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Material = m_Materials["stone0"].get();
		rightSphereRitem->Geometry = m_Geometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		m_RenderItems.push_back(std::move(leftCylRitem));
		m_RenderItems.push_back(std::move(rightCylRitem));
		m_RenderItems.push_back(std::move(leftSphereRitem));
		m_RenderItems.push_back(std::move(rightSphereRitem));
	}

	// All the render items are opaque.
	for (auto& e : m_RenderItems)
		m_OpaqueRenderItems.push_back(e.get());
}

void LightningD3DApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = CalculateConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = m_CurrentFrameResource->ObjectCB->GetResource();
	auto matCB = m_CurrentFrameResource->MaterialCB->GetResource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geometry->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geometry->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Material->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}