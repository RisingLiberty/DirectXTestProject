#include "D3DAppBase.h"

#include "FrameResource.h"
#include <iostream>

using namespace DirectX;

D3DAppBase::D3DAppBase(HINSTANCE hInstance):
	D3DApp(hInstance)
{

}

void D3DAppBase::Update(float dTime)
{
	// Nothing to implement
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

void D3DAppBase::Draw()
{
	// Nothing to implement
}

void D3DAppBase::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*XM_PI, this->GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);

}

void D3DAppBase::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(m_WindowHandle);
}

void D3DAppBase::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3DAppBase::OnMouseMove(WPARAM btnState, int x, int y)
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

void D3DAppBase::UpdateCamera()
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

void D3DAppBase::UpdateObjectCBs(const GameTimer& gt)
{
		auto currObjectCB = m_CurrentFrameResource->ObjectCB.get();
		for (auto& e : m_RenderItems)
		{
			// Only update the cbuffer data if the constants have changed.  
			// This needs to be tracked per frame resource.
			if (e->NumFramesDirty > 0)
			{
				XMMATRIX world = XMLoadFloat4x4(&e->World);
				XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
	
				ObjectConstants objConstants;
				XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
	
				currObjectCB->CopyData(e->ObjCBIndex, objConstants);
	
				// Next FrameResource need to be updated too.
				e->NumFramesDirty--;
			}
		}
}

void D3DAppBase::UpdateMainPassCB(const GameTimer& gt)
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

