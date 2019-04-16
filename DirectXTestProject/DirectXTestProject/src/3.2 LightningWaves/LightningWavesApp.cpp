#include "LightningWavesApp.h"
#include "1.0 Core/GeometryGenerator.h"

#include <DirectXColors.h>

#include <iostream>
using namespace DirectX;

LightningWavesApp::LightningWavesApp(HINSTANCE hInstance) :
	D3DAppBase(hInstance)
{
}

HRESULT LightningWavesApp::Initialize()
{
	ThrowIfFailed(D3DAppBase::Initialize());

	// Reset the command list to prep for initialization commands
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.
	// This is hardware specific, so we have to query this information
	m_CbvSrvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_Waves = std::make_unique<LightningWaves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	this->BuildRootSignature();
	this->BuildShadersAndInputLayout();
	this->BuildLandGeometry();
	this->BuildWavesGeometryBuffers();
	this->BuildMaterials();
	this->BuildRenderItems();
	this->BuildFrameResources();
	this->BuildPsos();

	// Execute the initializations commands
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Wait until initialization is complete
	this->FlushCommandQueue();

	return S_OK;
}

void LightningWavesApp::Update(const float dTime)
{
	D3DAppBase::Update(dTime);

	this->OnKeyboardInput(m_GameTimer);
	this->UpdateMaterialCBs(m_GameTimer);
	this->UpdateWaves(m_GameTimer);
}

void LightningWavesApp::Draw()
{
	D3DAppBase::Draw();

	using Microsoft::WRL::ComPtr;
	ComPtr<ID3D12CommandAllocator> cmdListAlloc = m_CurrentFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList
	// Reusing the command list reuses memory
	ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_Psos["opaque"].Get()));

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	// Indicate a state transition on the resource usage
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// Specifyt the buffers we are going to render to
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &GetDepthStencilView());

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	ID3D12Resource* passCB = m_CurrentFrameResource->PassCB->GetResource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	this->DrawRenderItems(m_CommandList.Get(), m_OpaqueRenderItems);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution
	ID3D12CommandList* cmdList[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(1, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	m_CurrentFrameResource->Fence = ++m_CurrentFence;

	// Add an instruction to the command queue to set a new fence point
	// Because we are on the GPU timeline, the new fence point won't be
	// set until the GPU finishes processing all the commands prior to this Signal()
	m_pCommandQueue->Signal(m_pFence.Get(), m_CurrentFence);
}

void LightningWavesApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_View);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	m_MainPassCB.EyePosW = m_EyePos;
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)WIDTH, (float)HEIGHT);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / WIDTH, 1.0f / HEIGHT);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.GetGameTime();
	m_MainPassCB.DeltaTime = gt.GetGameTime();
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_MainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	m_MainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	m_MainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = m_CurrentFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}


void LightningWavesApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.GetDeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		m_SunTheta -= 1.0f * dt;

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		m_SunTheta += 1.0f * dt;

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		m_SunPhi -= 1.0f * dt;

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		m_SunPhi += 1.0f * dt;

	m_SunPhi = Clamp(m_SunPhi, 0.1f, XM_PIDIV2);
}

void LightningWavesApp::BuildRootSignature()
{
	using Microsoft::WRL::ComPtr;

	// Root parameter can be a table, root desriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Create root CBV.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsConstantBufferView(2);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		std::cout << (char*)errorBlob->GetBufferPointer();

	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())
	));
}

void LightningWavesApp::BuildShadersAndInputLayout()
{
	m_Shaders["standardVS"] = CompileShader(L"../data/shaders/src/shader_lightning.fx", nullptr, "VS", "vs_5_0");
	m_Shaders["opaquePS"] = CompileShader(L"../data/shaders/src/shader_lightning.fx", nullptr, "PS", "ps_5_0");

	m_InputLayout =
	{{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	}};
}

void LightningWavesApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pDevice.Get(),
			1, (UINT)m_RenderItems.size(), (UINT)m_Materials.size(), m_Waves->GetVertexCount()));
	}
}

void LightningWavesApp::BuildPsos()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;

	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	opaqueDesc.pRootSignature = m_RootSignature.Get();
	opaqueDesc.VS = 
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};

	opaqueDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};

	opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueDesc.SampleMask = UINT_MAX;
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = m_BackBufferFormat;
	opaqueDesc.SampleDesc.Count = m_4xMsaaEnabled ? 4 : 1;
	opaqueDesc.SampleDesc.Quality = m_4xMsaaEnabled ? (m_4xMsaaQuality - 1) : 0;
	opaqueDesc.DSVFormat = m_DepthStencilFormat;

	ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&m_Psos["opaque"])));
}

void LightningWavesApp::BuildMaterials()
{
	std::unique_ptr<Material> grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering tools
	// we need (transparency, environment reflection), so we fke it for now).
	std::unique_ptr<Material> water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
}

void LightningWavesApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>();
	wavesRenderItem->World = MAT_4_IDENTITY;
	wavesRenderItem->ObjCBIndex = 0;
	wavesRenderItem->Material = m_Materials["water"].get();
	wavesRenderItem->Geometry = m_Geometries["waterGeo"].get();
	wavesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRenderItem->IndexCount = wavesRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	wavesRenderItem->StartIndexLocation = wavesRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	wavesRenderItem->BaseVertexLocation = wavesRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_WavesRenderItem = wavesRenderItem.get();

	m_OpaqueRenderItems.push_back(wavesRenderItem.get());

	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
	gridRenderItem->World = MAT_4_IDENTITY;
	gridRenderItem->ObjCBIndex = 1;
	gridRenderItem->Material = m_Materials["grass"].get();
	gridRenderItem->Geometry = m_Geometries["landGeo"].get();
	gridRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_OpaqueRenderItems.push_back(gridRenderItem.get());

	m_RenderItems.push_back(std::move(wavesRenderItem));
	m_RenderItems.push_back(std::move(gridRenderItem));
}

void LightningWavesApp::UpdateMaterialCBs(const GameTimer& gt)
{
	UploadBuffer<MaterialConstants>* currMaterialCB = m_CurrentFrameResource->MaterialCB.get();
	for (auto& e : m_Materials)
	{
		// Only update the cbuffer data if the constants have changed.
		// If the cbuffer data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too
			mat->NumFramesDirty--;
		}
	}
}

void LightningWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems)
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = CalculateConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrentFrameResource->ObjectCB->GetResource();
	ID3D12Resource* matCB = m_CurrentFrameResource->MaterialCB->GetResource();

	// For each render item..
	for (RenderItem* pRenderItem : renderItems)
	{
		cmdList->IASetVertexBuffers(0, 1, &pRenderItem->Geometry->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&pRenderItem->Geometry->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(pRenderItem->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCbAddress = objectCB->GetGPUVirtualAddress() + pRenderItem->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + pRenderItem->Material->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCbAddress);
		cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

		cmdList->DrawIndexedInstanced(pRenderItem->IndexCount, 1, pRenderItem->StartIndexLocation, pRenderItem->BaseVertexLocation, 0);
	}
}

void LightningWavesApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;

	if ((m_GameTimer.GetGameTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = Rand(4, m_Waves->GetRowCount() - 5);
		int j = Rand(4, m_Waves->GetColumnCount() - 5);

		float r = RandF(0.2f, 0.5f);

		m_Waves->Disturb(i, j, r);
	}

	// Update the wave simulation
	m_Waves->Update(gt.GetDeltaTime());

	// Update the wave vertex buffer with the new solution
	UploadBuffer<LightningVertex>* currWaveVB = m_CurrentFrameResource->WavesVB.get();
	for (int i = 0; i < m_Waves->GetVertexCount(); ++i)
	{
		LightningVertex v;
		
		v.Pos = m_Waves->GetPosition(i);
		v.Normal = m_Waves->GetNormal(i);

		currWaveVB->CopyData(i, v);
	}

	// Set the dynamic vb of the wave renderitem to the current frame VB.
	m_WavesRenderItem->Geometry->VertexBufferGPU = currWaveVB->GetResource();
}

void LightningWavesApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex. In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills and snow mountain peaks.

	std::vector<LightningVertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightningVertex);

	std::vector<uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(),
		m_CommandList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);

	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(LightningVertex);
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

void LightningWavesApp::BuildWavesGeometryBuffers()
{
	std::vector<uint16_t> indices(3 * m_Waves->GetTriangleCount()); // 3 indices per face
	assert(m_Waves->GetVertexCount() < 0x0000ffff);

	// ITerate over each quad
	int rowCount = m_Waves->GetRowCount();
	int columnCount = m_Waves->GetColumnCount();
	int k = 0;

	for (int row = 0; row < rowCount - 1; ++row)
	{
		for (int column = 0; column < columnCount - 1; ++column)
		{
			indices[k] = row * rowCount + column;
			indices[k + 1] = row * rowCount + column + 1;
			indices[k + 2] = (row + 1)*rowCount + column;

			indices[k + 3] = (row + 1)*rowCount + column;
			indices[k + 4] = row * rowCount + column + 1;
			indices[k + 5] = (row + 1)*rowCount + column + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = m_Waves->GetVertexCount() * sizeof(LightningVertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "waterGeo";

	// Set dynamically
	geometry->VertexBufferCPU = nullptr;
	geometry->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(LightningVertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geometry->DrawArgs["grid"] = submesh;

	m_Geometries["waterGeo"] = std::move(geometry);
}

float LightningWavesApp::GetHillsHeight(float x, float z)
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f*z));
}

DirectX::XMFLOAT3 LightningWavesApp::GetHillsNormal(float x, float z)
{
	XMFLOAT3 rowCount(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f *x*sinf(0.1f *z)
	);

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&rowCount));
	XMStoreFloat3(&rowCount, unitNormal);

	return rowCount;
}