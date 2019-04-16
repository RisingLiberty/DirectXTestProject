#include <DirectXColors.h>
#include <iostream>

#include "InitD3DApp.h"

#include "1.0 Core/Utils.h"

InitD3DApp::InitD3DApp(HINSTANCE instance) :
	D3DApp(instance)
{

}

void InitD3DApp::Update(const float dTime)
{

}

void InitD3DApp::Draw()
{
	//Reuse the memory associated with command recording.
	//We can only reset when the assocaited command lists have finished exection on the CPU.
	ThrowIfFailed(m_DirectCmdListAlloc->Reset());

	//A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	//Reusing the command list resuses memory.
	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	//Indicate a state transition on teh resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//Set the viewport and scissor rect. This needs to be reset whenever the command list is reset.
	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);
	
	//RenderTargetView: RTV to the resource we want to clear
	//ColorRGBA: Defines the color to clear the render target to
	//NumRects: The number of elements in the pRects array.
	//pRects: an array of D3D12_RECTs that identify rectangle regions on the render target to clear.
	//This can be nullptr to indicate to clear the entire render target.

	//float clearColor[4] = 
	//{
	//	0,0,0,1
	//};
	//
	//static float time = -1;
	//time += 0.001f;
	//float sinOutput = sin(time);
	//
	//sinOutput *= 0.5f;
	//sinOutput += 0.5f;
	//
	//clearColor[1] = sinOutput;
	//
	//RECT r[2] = 
	//{ 
	//	{ 0,0, 200, 300 },
	//	{ 100, 100, 500, 1000}
	//};
	//m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), clearColor, 2, r);

	//Clear the back buffer and depth buffer
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);

	//DepthStenciView: DSV to the depth/stencil buffer to clear
	//ClearFlags: Flags indicating which part of the depth/stencil buffer to clear.
	//This can be either D3D12_CLEAR_FLAG_DEPTH, D3D12_CLEAR_FLAG_STENCIL or both bitwised ORed together
	//Depth: Defines the value to clear the depth values to
	//Stencil: Defines the value to clear the stencil values to.
	//NumRects: the number of elements in the pREcts array. this can be 0
	//pRects: an array of D3D12_RECTs that identify rectangle regions on the render target to clear. 
	//This can be nullptr to indicate to clear the entire render target.
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//NumRenderTargetDescriptors: Specifies the number of RTVs we are going to bind.
	//Using multiple render targets simultaneously is used for more advanced techniques, for now, one is enough
	//pRenderTargetDescriptors: pointer to an array of RTVs that specify the render targets we want to bind to the pipeline.
	//RtsSingleHandleToDescriptorRange: specify true if all the RTVs in the previous array are contiguous in
	//the descriptor heap. Otherwise, specify false.
	//pDepthStencilDescriptor: pointer to a DSV that specifies the depth/stencil buffer we want to bind to the pipeline.

	//Specify the buffers we are going to render to.
	m_CommandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true, &this->GetDepthStencilView());

	//Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(this->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//Done recording commands.
	ThrowIfFailed(m_CommandList->Close());

	//Add the command list to the queue for execution
	ID3D12CommandList* cmdsList[] = { m_CommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsList), cmdsList);

	//Swap the back and front buffers
	ThrowIfFailed(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_SwapChainBufferCount;

	//Wait untill frame commands are complete. This waiting is inefficient and is
	//done for simplicity.
	this->FlushCommandQueue();

}	