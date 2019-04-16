#pragma once

#include "1.0 Core/D3DAppBase.h"
#include "LightningWaves.h"

class LightningWavesApp : public D3DAppBase
{
public:
	LightningWavesApp(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;

	void Update(const float dTime) override;
	void Draw() override;

private:
	void BuildRootSignature() override;
	void BuildShadersAndInputLayout() override;
	void BuildFrameResources() override;
	void BuildPsos() override;

	void BuildMaterials();
	void BuildRenderItems();
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);

	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

	float GetHillsHeight(float x, float z);
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z);

private:
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::vector<RenderItem*> m_OpaqueRenderItems;

	std::array<D3D12_INPUT_ELEMENT_DESC, 2> m_InputLayout;

	std::unique_ptr<LightningWaves> m_Waves;
	RenderItem* m_WavesRenderItem = nullptr;

	float m_SunTheta = 1.25f * DirectX::XM_PI;
	float m_SunPhi = DirectX::XM_PIDIV4;
};