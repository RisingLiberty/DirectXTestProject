#pragma once

#include "1.0 Core/D3DAppBase.h"

class LightningD3DApp : public D3DAppBase
{
public:
	LightningD3DApp(HINSTANCE hInstance);

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
	void BuildShapeGeometry();
	void BuildRenderItems();
	void BuildSkullGeometry();

	void UpdateMaterialCBs(const GameTimer& gt);
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);

private:
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::vector<RenderItem*> m_OpaqueRenderItems;

	std::array<D3D12_INPUT_ELEMENT_DESC, 3> m_InputLayout;

};

