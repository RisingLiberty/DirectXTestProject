#include "D3DAppBase.h"

using namespace DirectX;

D3DAppBase::D3DAppBase(HINSTANCE hInstance):
	D3DApp(hInstance)
{

}

void D3DAppBase::Update(float dTime)
{
	// Nothing to implement
	this->UpdateCamera();
}

void D3DAppBase::Draw(float dTime)
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