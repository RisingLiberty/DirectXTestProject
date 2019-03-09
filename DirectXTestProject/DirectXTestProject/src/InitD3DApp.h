#pragma once

#include "D3DApp.h"

class InitD3DApp final: public D3DApp
{
public:
	InitD3DApp(HINSTANCE instance);

	void Update(const float dTime) override;
	void Draw() override;
};