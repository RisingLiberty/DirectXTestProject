#include <DirectXColors.h>

#include "InitD3DApp.h"

#include "Utils.h"

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
	m_CommandList->RSSetScissorRects(1, &m_ScissorsRect);

	//Cleaar the back buffer and depth buffer
	m_CommandList->ClearRenderTargetView(this->GetCurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

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