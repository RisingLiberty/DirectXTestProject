#pragma once

#include "D3DApp.h"

#include <unordered_map>

#include "Utils.h"

#include "FrameResource.h"

class D3DAppBase : public D3DApp
{
public:
	D3DAppBase(HINSTANCE hInstance);

protected:
	virtual void Update(float dTime);
	virtual void Draw(float dTime);

	virtual void OnResize();

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	virtual void BuildFrameResources() = 0;
	virtual void BuildRootSignature() = 0;
	virtual void BuildDescriptorHeaps() = 0;
	virtual void BuildPsos() = 0;
	virtual void BuildShadersAndInputLayout() = 0;

	virtual void UpdateObjectCBs(const GameTimer& gt);
	virtual void UpdateMainPassCB(const GameTimer& gt);


protected:
	DirectX::XMFLOAT3 m_EyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 m_View = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvHeap;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	POINT m_LastMousePos;

	float m_Theta = 1.5f * DirectX::XM_PI;
	float m_Phi = 0.2f * DirectX::XM_PI;
	float m_Radius = 15.0f;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_Psos;
	std::array<D3D12_INPUT_ELEMENT_DESC, 2> m_InputLayout;

	FrameResource* m_CurrentFrameResource = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;

	static const int s_NumFrameResources = gNumFrameResources;
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	int m_CurrentFrameResourceIndex = 0;

	PassConstants m_MainPassCB;

	std::vector<std::unique_ptr<RenderItem>> m_RenderItems;

private:
	void UpdateCamera();
};