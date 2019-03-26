#pragma once

#include "1.0 Core/D3DAppBase.h"

enum class RenderLayer : char
{
	Opaque = 0,
	
	Count
};

class Waves;

class DrawingD3DAppIII : public D3DAppBase
{
public:
	DrawingD3DAppIII(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;
	void Update(float dTime) override;
	void Draw() override;

	void BuildFrameResources() override;
	void BuildRootSignature() override;
	void BuildDescriptorHeaps() override;
	void BuildPsos() override;
	void BuildShadersAndInputLayout() override;

private:
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	float GetHillsHeight(float x, float z) const;
	void DrawRenderItems(ID3D12GraphicsCommandList* pCommandList, const std::vector<RenderItem*>& renderItems);

private:
	std::vector<RenderItem*> m_RenderItemLayer[(int)RenderLayer::Count];
	
	std::unique_ptr<Waves> m_Waves;

};