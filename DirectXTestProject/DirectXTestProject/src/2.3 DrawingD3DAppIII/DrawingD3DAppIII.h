#pragma once

#include "1.0 Core/D3DAppBase.h"
#include "Waves.h"

enum class RenderLayer : char
{
	Opaque = 0,
	
	Count
};


class DrawingD3DAppIII : public D3DAppBase
{
public:
	DrawingD3DAppIII(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;
	void Update(float dTime) override;
	void Draw() override;

	void BuildRootSignature() override;
	void BuildShadersAndInputLayout() override;
	void BuildFrameResources() override;
	void BuildPsos() override;

private:
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildRenderItems();
	void BuildDescriptorHeaps();

	void DrawRenderItems(ID3D12GraphicsCommandList* pCommandList, const std::vector<RenderItem*>& renderItems);

	void UpdateWaves(const GameTimer& gt);
	float GetHillsHeight(float x, float z) const;

private:
	std::vector<RenderItem*> m_RenderItemLayer[(int)RenderLayer::Count];
	
	std::unique_ptr<Waves> m_Waves;

	bool m_IsWireframe;
	RenderItem* m_WavesRenderItem = nullptr;

	std::array<D3D12_INPUT_ELEMENT_DESC, 2> m_InputLayout;

};