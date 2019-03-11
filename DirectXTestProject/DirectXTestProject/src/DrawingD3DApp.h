#pragma once

#include "D3DApp.h"


class DrawingD3DApp : public D3DApp
{
public:
	DrawingD3DApp(HINSTANCE hInstance);

protected:
	virtual HRESULT Initialize() override;

private:
	std::array<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
};