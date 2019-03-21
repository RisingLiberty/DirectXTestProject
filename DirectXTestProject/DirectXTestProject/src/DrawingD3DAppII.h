#pragma once

#include "D3DApp.h"

#include "FrameResource.h"

#include <memory>
#include <array>
#include <unordered_map>


class DrawingD3DAppII : public D3DApp
{
public:
	DrawingD3DAppII(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;

	void Update(const float dTime) override;
	void Draw() override;
	
	virtual void OnResize();

private:
	void BuildFrameResources();
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void BuildRootSignature();
	void BuildShapeGeometry();
	void BuildRenderItems();
	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildPsos();
	void BuildShadersAndInputLayout();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);

private:
	static const int s_NumFrameResources = gNumFrameResources;
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrentFrameResource = nullptr;
	int m_CurrentFrameResourceIndex = 0;

	// Our application will maintain lists of render items based on how they need to be drawn.
	// that is, render items that need differens PSOs will be kept in different lists.
	std::vector<std::unique_ptr<RenderItem>> m_RenderItems;
	std::vector<RenderItem*> m_OpaqueRenderItems;

	//// Render items divided by PSO.
	//std::vector<RenderItem*> m_OpaqueRenderItems;
	//std::vector<RenderItem*> m_TransparentRenderItems;

	DirectX::XMFLOAT3 m_EyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 m_View = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvHeap;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;


	PassConstants m_MainPassCB;

	UINT m_PassCbvOffset;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_Psos;
	std::array<D3D12_INPUT_ELEMENT_DESC, 2> m_InputLayout;


};