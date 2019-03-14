#pragma once

#include "D3DApp.h"

#include "FrameResource.h"

class DrawingD3DAppII : public D3DApp
{
public:
	DrawingD3DAppII(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;

	void Update(const float dTime) override;
	void Draw() override;
	
private:
	void BuildFrameResources();
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void BuildRootSignature();
private:
	static const int s_NumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrentFrameResource = nullptr;
	int m_CurrentFrameResourceIndex = 0;

	// Our application will maintain lists of render items based on how they need to be drawn.
	// that is, render items that need differens PSOs will be kept in different lists.
	std::vector<std::unique_ptr<RenderItem>> m_RenderItems;

	// Render items divided by PSO.
	std::vector<RenderItem*> m_OpaqueRenderItems;
	std::vector<RenderItem*> m_TransparentRenderItems;

	XMFLOAT3 m_EyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 m_View = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 m_Proj = MAT_4_IDENTITY;

	PassConstants m_MainPassCB;

};