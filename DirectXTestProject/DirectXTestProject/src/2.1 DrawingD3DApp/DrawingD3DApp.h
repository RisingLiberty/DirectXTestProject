#pragma once

#include "1.0 Core/D3DApp.h"
#include "1.0 Core/UploadBuffer.h"

#include "1.0 Core/Utils.h"

class DrawingD3DApp : public D3DApp
{
public:
	DrawingD3DApp(HINSTANCE hInstance);

protected:
	virtual HRESULT Initialize() override;

	virtual void OnResize() override;

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
	float m_Theta = 1.5f * DirectX::XM_PI;
	float m_Phi = DirectX::XM_PIDIV4;
	float m_Radius = 5.0f;

	POINT m_LastMousePos;

	// Constant buffer to store the constants of n object.
	std::unique_ptr<UploadBuffer<ObjectConstants>> m_ObjectConstantBuffer = nullptr;
	
	DirectX::XMFLOAT4X4 m_World = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 m_View = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvHeap;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
	Microsoft::WRL::ComPtr<ID3DBlob> m_VsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_PsByteCode = nullptr;

	std::unique_ptr<MeshGeometry> m_BoxGeo = nullptr;

	std::array<D3D12_INPUT_ELEMENT_DESC, 2> m_InputLayout;
};
