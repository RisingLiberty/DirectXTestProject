#include "DrawingD3DAppII.h"

#include "Utils.h"

using namespace DirectX;

DrawingD3DAppII::DrawingD3DAppII(HINSTANCE hInstance):
	D3DApp(hInstance)
{
	m_AppName = L"Drawing D3D App II";
}

HRESULT DrawingD3DAppII::Initialize()
{
	ThrowIfFailed(D3DApp::Initialize());

	this->BuildFrameResources();
	
}

void DrawingD3DAppII::Update(const float dTime)
{
	// Cycle through the circular frame resource array.
	m_CurrentFrameResourceIndex = (m_CurrentFrameResourceIndex + 1) % s_NumFrameResources;
	m_CurrentFrameResource = m_FrameResources[m_CurrentFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait untill the GPU has completed commands up to this fence point.
	if (m_CurrentFrameResource->Fence != 0 && m_pFence->GetCompletedValue() < m_CurrentFrameResource->Fence);
	{
		HANDLE eventHandle = CreateEventExW(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_pFence->SetEventOnCompletion(m_CurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// TODO: Update resources in m_CurrentFrameResource (like cbuffers).
}

void DrawingD3DAppII::Draw()
{
	// TODO: Buold and submit command lists for this frame.

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
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(slotRootParameter.size(), slotRootParameter.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Note: do not go overboard with the number of constant buffers in your shaders.
	// [Thibieroz13] recommends you keep them under 5 for performance.
}