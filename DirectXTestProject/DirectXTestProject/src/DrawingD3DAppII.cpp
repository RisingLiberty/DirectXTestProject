#include "DrawingD3DAppII.h"
#include "GeometryGenerator.h"

#include <DirectXColors.h>

#include "Utils.h"

using namespace DirectX;
using namespace Microsoft::WRL;

DrawingD3DAppII::DrawingD3DAppII(HINSTANCE hInstance):
	D3DApp(hInstance)
{
	m_AppName = L"Drawing D3D App II";
}

HRESULT DrawingD3DAppII::Initialize()
{
	ThrowIfFailed(D3DApp::Initialize());
	
	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	this->BuildRootSignature();
	this->BuildShadersAndInputLayout();
	this->BuildShapeGeometry();
	this->BuildRenderItems();
	this->BuildFrameResources();
	this->BuildDescriptorHeaps();
	this->BuildConstantBufferViews();
	this->BuildPsos();
	
	// Execute the initialization commands
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Wait untill initialization is complete
	this->FlushCommandQueue();

	return S_OK;
}

void DrawingD3DAppII::Update(const float dTime)
{
	this->UpdateCamera();

	// Cycle through the circular frame resource array.
	m_CurrentFrameResourceIndex = (m_CurrentFrameResourceIndex + 1) % s_NumFrameResources;
	m_CurrentFrameResource = m_FrameResources[m_CurrentFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait untill the GPU has completed commands up to this fence point.
	if (m_CurrentFrameResource->Fence != 0 && m_pFence->GetCompletedValue() < m_CurrentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventExW(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_pFence->SetEventOnCompletion(m_CurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	this->UpdateObjectCBs(m_GameTimer);
	this->UpdateMainPassCB(m_GameTimer);
}

void DrawingD3DAppII::Draw()
{
	// TODO: Build and submit command lists for this frame.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdListAlloc = m_CurrentFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_Psos["opaque"].Get()));

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorsRect);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear the back buffer and depth buffer.
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &this->GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	int passCbvIndex = m_PassCbvOffset + m_CurrentFrameResourceIndex;
	CD3DX12_GPU_DESCRIPTOR_HANDLE passCbvHandle(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, m_CbvSrvDescriptorSize);
	m_CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	this->DrawRenderItems(m_CommandList.Get(), m_OpaqueRenderItems);

	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(1, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	m_CurrentFrameResource->Fence = ++m_CurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on GPU timeline, the new fence point won't be set until the GPU
	// finishes processing all the commands prior to this Signal().
	m_pCommandQueue->Signal(m_pFence.Get(), m_CurrentFence);

	// Note that GPU could still be working on commands from previous frames,
	// but that is okay, because we are not touching any frame resources associated
	// with those frames.

	// Note that this solution does not prevent waiting.
	// If one processor is processing frames much faster than the other, 
	// one processor will eventually have to wait for the other to catch up, 
	// as we cannot let one get too far ahead of the other.
	// If the GPU is processing commands faster than the CPU can submit work, 
	// then the GPU will idle.In general, if we are trying to push the graphical limit, 
	// we want to avoid this situation, as we are not taking full advantage of the GPU.
	// On the other hand, if the CPU is always processing frames faster than the GPU, 
	// then the CPU will have to wait at some point.
	// This is the desired situation, as the GPU is being fully utilized.
	// The extra CPU cycles can always be used for other parts of the game such as AI, physics, and game play logic.
}

void DrawingD3DAppII::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*XM_PI, this->GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void DrawingD3DAppII::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(m_WindowHandle);
}

void DrawingD3DAppII::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();

}

void DrawingD3DAppII::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_LastMousePos.y));

		// Update angles based on input to orbit camera around box.
		m_Theta += dx;
		m_Phi += dy;

		// Restrict the angle m_Phi.
		m_Phi = Clamp(m_Phi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f*static_cast<float>(x - m_LastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - m_LastMousePos.y);

		// Update the camera radius based on input.
		m_Radius += dx - dy;

		// Restrict the radius.
		m_Radius = Clamp(m_Radius, 5.0f, 150.0f);
	}

	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void DrawingD3DAppII::BuildFrameResources()
{
	for (int i = 0; i < s_NumFrameResources; ++i)
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pDevice.Get(), 1, (UINT)m_RenderItems.size()));
}

void DrawingD3DAppII::UpdateObjectCBs(const GameTimer& gt)
{
	UploadBuffer<ObjectConstants>* currentObjectCB = m_CurrentFrameResource->ObjectCB.get();
	
	for (const std::unique_ptr<RenderItem>& pRenderItem : m_RenderItems)
	{
		// Only update the cbuffer data if the constants have changed.
		// This needs to be tracked per frame resource.
		if (pRenderItem->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&pRenderItem->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currentObjectCB->CopyData(pRenderItem->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too
			pRenderItem->NumFramesDirty--;
		}
	}
}

void DrawingD3DAppII::UpdateMainPassCB(const GameTimer& gt)
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
	m_MainPassCB.DeltaTime = gt.GetDeltaTime();

	UploadBuffer<PassConstants>* currPassCB = m_CurrentFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void DrawingD3DAppII::BuildRootSignature()
{
	// The resources that our shaders expect have changed, therefore, we need to update
	// the root signature accordingly to take 2 descriptor tables (we need 2 tables because
	// the CBVs will be set at different frequencies -- the per pass CBV only needs to be set once
	// per rendering pass, while the per object CBV needs to be set per render item).

	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// Root parametere can be a tbale, root descriptor or root constants.
	std::array<CD3DX12_ROOT_PARAMETER,2> slotRootParameter;

	// Create root CBVs.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(static_cast<UINT>(slotRootParameter.size()), slotRootParameter.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Note: do not go overboard with the number of constant buffers in your shaders.
	// [Thibieroz13] recommends you keep them under 5 for performance.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf())));
}

void DrawingD3DAppII::BuildShapeGeometry()
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

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}
	
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}

	std::vector<uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "shapeGeo";
	
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	memcpy(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	memcpy(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);
	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(Vertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	geometry->DrawArgs["box"] = boxSubmesh;
	geometry->DrawArgs["grid"] = gridSubmesh;
	geometry->DrawArgs["sphere"] = sphereSubMesh;
	geometry->DrawArgs["cylinder"] = cylinderSubmesh;


	m_Geometries[geometry->Name] = std::move(geometry);
}

void DrawingD3DAppII::BuildRenderItems()
{
	std::unique_ptr<RenderItem> boxRenderItem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRenderItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRenderItem->ObjCBIndex = 0;
	boxRenderItem->Geometry = m_Geometries["shapeGeo"].get();
	boxRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRenderItem->IndexCount = boxRenderItem->Geometry->DrawArgs["box"].IndexCount;
	boxRenderItem->StartIndexLocation = boxRenderItem->Geometry->DrawArgs["box"].StartIndexLocation;
	boxRenderItem->BaseVertexLocation = boxRenderItem->Geometry->DrawArgs["box"].BaseVertexLocation;
	
	m_RenderItems.push_back(std::move(boxRenderItem));

	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();

	gridRenderItem->World = MAT_4_IDENTITY;
	gridRenderItem->ObjCBIndex = 1;
	gridRenderItem->Geometry = m_Geometries["shapeGeo"].get();
	gridRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geometry->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geometry->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	m_RenderItems.push_back(std::move(gridRenderItem));

	std::unique_ptr<RenderItem> sphereRenderItem = std::make_unique<RenderItem>();

	UINT objCbiIndex = 2;
	for (int i = 0; i < 5; ++i)
	{
		std::unique_ptr<RenderItem> leftCylinderRenderItem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightCylinderRenderItem = std::make_unique<RenderItem>();

		std::unique_ptr<RenderItem> leftSphereRenderItem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightSphereRenderItem = std::make_unique<RenderItem>();

		const float offset = 5.0f;

		XMMATRIX leftCylinderWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * offset);
		XMMATRIX rightCylinderWorld = XMMatrixTranslation(5.0f, 1.5f, -10.0f + i * offset);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * offset);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(5.0f, 3.5f, -10.0f + i * offset);

		XMStoreFloat4x4(&leftCylinderRenderItem->World, leftCylinderWorld);
		leftCylinderRenderItem->ObjCBIndex = objCbiIndex++;
		leftCylinderRenderItem->Geometry = m_Geometries["shapeGeo"].get();
		leftCylinderRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRenderItem->IndexCount = leftCylinderRenderItem->Geometry->DrawArgs["cylinder"].IndexCount;
		leftCylinderRenderItem->StartIndexLocation = leftCylinderRenderItem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		leftCylinderRenderItem->BaseVertexLocation = leftCylinderRenderItem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;
		
		XMStoreFloat4x4(&leftSphereRenderItem->World, leftSphereWorld);
		leftSphereRenderItem->ObjCBIndex = objCbiIndex++;
		leftSphereRenderItem->Geometry = m_Geometries["shapeGeo"].get();
		leftSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRenderItem->IndexCount = leftSphereRenderItem->Geometry->DrawArgs["sphere"].IndexCount;
		leftSphereRenderItem->StartIndexLocation = leftSphereRenderItem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRenderItem->BaseVertexLocation = leftSphereRenderItem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylinderRenderItem->World, rightCylinderWorld);
		rightCylinderRenderItem->ObjCBIndex = objCbiIndex++;
		rightCylinderRenderItem->Geometry = m_Geometries["shapeGeo"].get();
		rightCylinderRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRenderItem->IndexCount = rightCylinderRenderItem->Geometry->DrawArgs["cylinder"].IndexCount;
		rightCylinderRenderItem->StartIndexLocation = rightCylinderRenderItem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		rightCylinderRenderItem->BaseVertexLocation = rightCylinderRenderItem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRenderItem->World, rightSphereWorld);
		rightSphereRenderItem->ObjCBIndex = objCbiIndex++;
		rightSphereRenderItem->Geometry = m_Geometries["shapeGeo"].get();
		rightSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRenderItem->IndexCount = rightSphereRenderItem->Geometry->DrawArgs["sphere"].IndexCount;
		rightSphereRenderItem->StartIndexLocation = rightSphereRenderItem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRenderItem->BaseVertexLocation = rightSphereRenderItem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		m_RenderItems.push_back(std::move(leftCylinderRenderItem));
		m_RenderItems.push_back(std::move(rightCylinderRenderItem));
		m_RenderItems.push_back(std::move(leftSphereRenderItem));
		m_RenderItems.push_back(std::move(rightSphereRenderItem));


	}

	// All render items are opaque
	for (const std::unique_ptr<RenderItem>& pRenderItem : m_RenderItems)
		m_OpaqueRenderItems.push_back(pRenderItem.get());

}

void DrawingD3DAppII::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)m_OpaqueRenderItems.size();

	// Need a CBV descriptor for each object for each frame resource.
	// +1 for the perPass CBV for each frame resource.

	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// Save an offset to the start of the pass CBVs. These are the last 3 desciptors
	m_PassCbvOffset = objCount * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_CbvHeap.ReleaseAndGetAddressOf())));
}

void DrawingD3DAppII::BuildConstantBufferViews()
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

	UINT objCount = (UINT)m_OpaqueRenderItems.size();

	// Need a CBV descriptor for each object for each frame resource
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		ID3D12Resource* objectCB = m_FrameResources[frameIndex]->ObjectCB->GetResource();
		for (UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the current buffer.
			cbAddress += i * objCBByteSize;

			// Offset to the object CBV in the descriptor heap.
			int heapIndex = frameIndex * objCount + i;
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, m_CbvSrvDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			m_pDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	UINT passCBByteSize = CalculateConstantBufferByteSize(sizeof(PassConstants));

	// Last three descriptors are the pass CBVs for each frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		ID3D12Resource* passCB = m_FrameResources[frameIndex]->PassCB->GetResource();

		// Pass buffer only stores on cbuffer per frame resource.
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset to the pass cbv in the descriptor heap.
		int heapIndex = m_PassCbvOffset + frameIndex;

		// Specify the number of descriptors to offset times the descriptor offset by n descriptors
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, m_CbvSrvDescriptorSize);
		//handle.Offset(heapIndex * m_CbvSrvDescriptorSize); // previous line is equal to this

		// Note: CD3DX12_GPU_DESCRIPTOR_HANDLE has the same Offset methods

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		m_pDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void DrawingD3DAppII::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems)
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

	ID3D12Resource* objectCB = m_CurrentFrameResource->ObjectCB->GetResource();

	// For each render ittem.
	for (RenderItem* renderItem : renderItems)
	{
		cmdList->IASetVertexBuffers(0, 1, &renderItem->Geometry->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&renderItem->Geometry->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(renderItem->PrimitiveType);

		// Offset to the CBV in the descriptor heap for this object and for this frame resource
		UINT cbvIndex = m_CurrentFrameResourceIndex * (UINT)m_OpaqueRenderItems.size() + renderItem->ObjCBIndex;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, m_CbvSrvDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		cmdList->DrawIndexedInstanced(renderItem->IndexCount, 1, renderItem->StartIndexLocation, renderItem->BaseVertexLocation, 0);
	}
}

void DrawingD3DAppII::BuildPsos()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// Pso for opaque objects
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
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
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
}

void DrawingD3DAppII::BuildShadersAndInputLayout()
{
	m_Shaders["standardVS"] = CompileShader(L"../data/shaders/src/shader_shapes.fx", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = CompileShader(L"../data/shaders/src/shader_shapes.fx", nullptr, "PS", "ps_5_1");

	m_InputLayout = 
	{{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	}};
}

void DrawingD3DAppII::UpdateCamera()
{
	// convert spherical to cartesian coordinates
	m_EyePos.x = m_Radius * sinf(m_Phi)*cosf(m_Theta);
	m_EyePos.z = m_Radius * sinf(m_Phi)*sinf(m_Theta);
	m_EyePos.y = m_Radius * cosf(m_Phi);

	// Build the view matrix
	XMVECTOR pos = XMVectorSet(m_EyePos.x, m_EyePos.y, m_EyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_View, view);
}