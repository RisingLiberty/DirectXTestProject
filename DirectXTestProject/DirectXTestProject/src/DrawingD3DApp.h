#pragma once

#include "D3DApp.h"
#include "UploadBuffer.h"

#define MAT_4_IDENTITY XMFLOAT4X4\
(\
	1.0f, 0.0f, 0.0f, 0.0f,\
	0.0f, 1.0f, 0.0f, 0.0f,\
	0.0f, 0.0f, 1.0f, 0.0f,\
	0.0f, 0.0f, 0.0f, 1.0f\
)

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = MAT_4_IDENTITY;
};

class DrawingD3DApp : public D3DApp
{
public:
	DrawingD3DApp(HINSTANCE hInstance);

protected:
	virtual HRESULT Initialize() override;

	virtual void Update(float dTime) override;
	virtual void Draw() override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaderAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	std::array<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	float m_Theta = 1.5f*XM_PI;
	float m_Phi = XM_PIDIV4;
	float m_Radius = 5.0f;

	POINT m_LastMousePos;

	// Constant buffer to store the constants of n object.
	std::unique_ptr<UploadBuffer<ObjectConstants>> m_ObjectConstantBuffer = nullptr;
	
	XMFLOAT4X4 m_World = MAT_4_IDENTITY;
	XMFLOAT4X4 m_View = MAT_4_IDENTITY;
	XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvHeap;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

};