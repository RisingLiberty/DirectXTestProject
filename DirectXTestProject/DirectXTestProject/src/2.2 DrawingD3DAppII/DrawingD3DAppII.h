#pragma once

#include "1.0 Core/D3DAppBase.h"

#include "1.0 Core/FrameResource.h"

#include <memory>
#include <array>
#include <unordered_map>


class DrawingD3DAppII : public D3DAppBase
{
public:
	DrawingD3DAppII(HINSTANCE hInstance);

protected:
	HRESULT Initialize() override;

	void Update(const float dTime) override;
	void Draw() override;
	
private:
	void BuildFrameResources() override;
	void BuildRootSignature() override;
	void BuildDescriptorHeaps() override;
	void BuildPsos() override;
	void BuildShadersAndInputLayout() override;

	void BuildShapeGeometry();
	void BuildRenderItems();
	void BuildConstantBufferViews();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);
private:

	// Our application will maintain lists of render items based on how they need to be drawn.
	// that is, render items that need differens PSOs will be kept in different lists.
	std::vector<RenderItem*> m_OpaqueRenderItems;

	//// Render items divided by PSO.
	//std::vector<RenderItem*> m_OpaqueRenderItems;
	//std::vector<RenderItem*> m_TransparentRenderItems;


	UINT m_PassCbvOffset;
};