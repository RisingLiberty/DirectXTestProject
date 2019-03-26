#include "DrawingD3DAppIII.h"

#include <DirectXColors.h>
#include <iostream>

#include "1.0 Core/GeometryGenerator.h"
#include "1.0 Core/FrameResource.h"

using namespace DirectX;

DrawingD3DAppIII::DrawingD3DAppIII(HINSTANCE hInstance) :
	D3DAppBase(hInstance)
{
	m_AppName = L"Drawing D3D App III";
}

HRESULT DrawingD3DAppIII::Initialize()
{
	ThrowIfFailed(D3DAppBase::Initialize());

	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	this->BuildRootSignature();
	this->BuildShadersAndInputLayout();
	this->BuildLandGeometry();
	this->BuildWavesGeometryBuffers();
	this->BuildRenderItems();
	this->BuildFrameResources();
	this->BuildPsos();

	// Execute the initialization commands
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Wait until initialization is complete
	this->FlushCommandQueue();

	m_IsWireframe = true;

	return S_OK;
}

void DrawingD3DAppIII::Update(float dTime)
{
	D3DAppBase::Update(dTime);

	this->UpdateWaves(m_GameTimer);
}

void DrawingD3DAppIII::Draw()
{
	using Microsoft::WRL::ComPtr;

	ComPtr<ID3D12CommandAllocator> cmdListAlloc = m_CurrentFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command quue via ExecuteCommandList.
	// Reusing the command list reusing memory.
	if (m_IsWireframe)
	{
		ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_Psos["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_Psos["opaque"].Get()));
	}

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorsRect);

	// Indicate a state transition on the resource usage
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &this->GetDepthStencilView());
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Bind per-pass constant buffer. We only need to do this once per-pass.
	ID3D12Resource* passCB = m_CurrentFrameResource->PassCB->GetResource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdList[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(1, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	// Advance the fence value to makrk commands up to this fence point.
	m_CurrentFrameResource->Fence = ++m_CurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set
	// untill the GPU finishes processing all the commands prior to this Signal().
	m_pCommandQueue->Signal(m_pFence.Get(), m_CurrentFence);
}

void DrawingD3DAppIII::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pDevice.Get(), 1, (UINT)m_RenderItems.size(), m_Waves->GetVertexCount()));
	}
}

void DrawingD3DAppIII::BuildRenderItems()
{
	std::unique_ptr<RenderItem> waveRenderItem = std::make_unique<RenderItem>();
	waveRenderItem->World = MAT_4_IDENTITY;
	waveRenderItem->ObjCBIndex = 0;
	waveRenderItem->Geometry = m_Geometries["waterGeo"].get();
	waveRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	waveRenderItem->IndexCount = waveRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	waveRenderItem->StartIndexLocation = waveRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	waveRenderItem->BaseVertexLocation = waveRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_WavesRenderItem = waveRenderItem.get();

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(waveRenderItem.get());

	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
	gridRenderItem->World = MAT_4_IDENTITY;
	gridRenderItem->ObjCBIndex = 1;
	gridRenderItem->Geometry = m_Geometries["landGeo"].get();
	gridRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());

	m_RenderItems.push_back(std::move(waveRenderItem));
	m_RenderItems.push_back(std::move(gridRenderItem));
}

void DrawingD3DAppIII::BuildRootSignature()
{
	// Root paramter can be a table, root descriptor or root constans.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// The following code specifies the shader register this paramter is bound to register b0 and b1)

	// Create root CBV.
	slotRootParameter[0].InitAsConstantBufferView(0); //Per-object CBV
	slotRootParameter[1].InitAsConstantBufferView(1); //Per-pass CBV

	//A root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//RootParndex: The index of the root paramtere we are bidning a CBV to.
	//BufferLocation: the virtual address to the resource that contains the constant buffer data.
	//ID3D12GRaphicsCommandList::SetGraphicsRootConstantBuffer(UINT RootParameterIndex, D3D12_GPU_VIRTUAL ADDRESS BufferLocation);
	using Microsoft::WRL::ComPtr;
	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
		serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(
		0, 
		serializedRootSignature->GetBufferPointer(), 
		serializedRootSignature->GetBufferSize(), 
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void DrawingD3DAppIII::BuildDescriptorHeaps()
{
	// Nothing to implement
}

void DrawingD3DAppIII::BuildShadersAndInputLayout()
{
	m_Shaders["standardVS"] = CompileShader(L"../data/shaders/src/shader_shapes.fx", nullptr, "VS", "vs_5_0");
	m_Shaders["opaquePS"] = CompileShader(L"../data/shaders/src/shader_shapes.fx", nullptr, "PS", "ps_5_0");

	m_InputLayout =
	{ {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	} };
}

void DrawingD3DAppIII::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	// Extract the vertex elements we are interested and apply the height function to each vertex
	// In addition, color the vertices based on their height so we have sandy looking beaches, grassy low hills,
	// and snow mountain peaks.
	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		Vertex& vertex = vertices[i];;

		vertex.Pos = p;
		vertex.Pos.y = GetHillsHeight(p.x, p.z);
		
		// Color the vertex based on its height.
		if (vertex.Pos.y < -10.0f)
		{
			// Sandy beach color
			vertex.Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (vertex.Pos.y < 5.0f)
		{
			// Light yellow-green
			vertex.Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (vertex.Pos.y < 12.0f)
		{
			// Dark yellow-green
			vertex.Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertex.Pos.y < 20.0f)
		{
			// Dark brown
			vertex.Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow
			vertex.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<uint16_t> indices = grid.GetIndices16();

	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);
	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(Vertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	geometry->DrawArgs["grid"] = subMesh;

	m_Geometries["landGeo"] = std::move(geometry);
}

void DrawingD3DAppIII::BuildWavesGeometryBuffers()
{
	std::vector<uint16_t> indices(3 * m_Waves->GetTriangleCount()); // 3 indices per face
	assert(m_Waves->GetVertexCount() < 0x0000ffff);

	// Iterate over each quad
	int rowCount = m_Waves->GetRowCount();
	int columnCount = m_Waves->GetColumnCount();
	int k = 0;
	for (int row = 0; row < rowCount - 1; ++row)
	{
		for (int column = 0; column < columnCount - 1; ++column)
		{
			indices[k + 0] = row * rowCount + column;
			indices[k + 1] = row * rowCount + column + 1;
			indices[k + 2] = (row + 1) * rowCount + column;

			indices[k + 3] = (row + 1) * rowCount + column;
			indices[k + 4] = row * rowCount + column + 1;
			indices[k + 5] = (row + 1) * rowCount + column + 1;

			k += 6; // move to next quad
		}
	}

	UINT vbByteSize = m_Waves->GetVertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "waterGeo";

	// Set dynamically
	geometry->VertexBufferCPU = nullptr;
	geometry->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(Vertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	geometry->DrawArgs["grid"] = subMesh;

	m_Geometries["waterGeo"] = std::move(geometry);
}

void DrawingD3DAppIII::BuildPsos()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// PSO for opaque objects
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

	ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_Psos["opaque"])));

	// PSO for opaque wireframe objects.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_Psos["opaque_wireframe"])));
}

float DrawingD3DAppIII::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z*sinf(0.1f * x) + x * cosf(0.1f * z));
}

void DrawingD3DAppIII::DrawRenderItems(ID3D12GraphicsCommandList* pCommandList, const std::vector<RenderItem*>& renderItems)
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

	ID3D12Resource* objectCB = m_CurrentFrameResource->ObjectCB->GetResource();

	// For each render item..
	for (RenderItem* pRenderItem : renderItems)
	{
		pCommandList->IASetVertexBuffers(0, 1, &pRenderItem->Geometry->GetVertexBufferView());
		pCommandList->IASetIndexBuffer(&pRenderItem->Geometry->GetIndexBufferView());
		pCommandList->IASetPrimitiveTopology(pRenderItem->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += pRenderItem->ObjCBIndex * objCBByteSize;

		pCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		pCommandList->DrawIndexedInstanced(pRenderItem->IndexCount, 1, pRenderItem->StartIndexLocation, pRenderItem->BaseVertexLocation, pRenderItem->StartIndexLocation);
	}
}

void DrawingD3DAppIII::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float baseTime = 0.0f;

	const float limit = 0.25f;

	if ((gt.GetGameTime() - baseTime) >= limit)
	{
		baseTime += limit;
		
		int i = Rand(4, m_Waves->GetRowCount() - 5);
		int j = Rand(4, m_Waves->GetColumnCount() - 5);

		float r = RandF(0.2f, 0.5f);

		m_Waves->Disturb(i, j, r);
	}

	// Update the wave simulation
	m_Waves->Update(gt.GetDeltaTime());

	UploadBuffer<Vertex>* currentWavesVB = m_CurrentFrameResource->WavesVB.get();

	for (int i = 0; i < m_Waves->GetVertexCount(); ++i)
	{
		Vertex v;

		v.Pos = m_Waves->GetPosition(i);
		v.Color = XMFLOAT4(DirectX::Colors::Blue);

		currentWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	m_WavesRenderItem->Geometry->VertexBufferGPU = currentWavesVB->GetResource();

	// Note: we save a reference to the wave render item so that we can set its vertex buffer on the fly.
	// We need to do this because its vertex buffer is a dynamic buffer and changes every frame.
}