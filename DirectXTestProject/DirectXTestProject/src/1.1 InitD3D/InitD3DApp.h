#pragma once

#include "1.0 Core/D3DApp.h"

class InitD3DApp final: public D3DApp
{
public:
	InitD3DApp(HINSTANCE instance);

	void Update(const float dTime) override;
	void Draw() override;
};