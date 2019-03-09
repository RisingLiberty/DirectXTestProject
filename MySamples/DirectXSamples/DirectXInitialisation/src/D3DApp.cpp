#include "D3DApp.h"

using namespace Microsoft::WRL;
using namespace DirectX;

D3DApp::D3DApp(HINSTANCE hInstance):
	m_AppInstance(hInstance),
	m_WindowHandle(nullptr),
	m_IsPaused(false),
	m_IsResizing(false),
	m_IsMultiSampling(false),
	m_MultiSamplingQuality(0),
	m_CurrentFence(0),
	m_RtvDescriptorSize(0),
	m_DsvDescriptorSize(0),
	m_CbvSrvUavDescriptorSize(0)
{
	
}

D3DApp::~D3DApp()
{
	if (m_Device)
		FlushCommandQueue();
}

void D3DApp::Start()
{
	this->Initialize();
	this->Run();
}

LRESULT D3DApp::WindowProcedureStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

bool D3DApp::IsMultiSampling() const
{
	return m_IsMultiSampling;
}

float D3DApp::GetAspectRatio() const
{
	return WIDTH / HEIGHT;
}

HWND D3DApp::GetMainWindow() const
{
	return m_WindowHandle;
}

void D3DApp::IsMultiSampling(bool isMultiSampling)
{
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
}

void D3DApp::OnResize()
{
}

void D3DApp::Update(const float dTime)
{
}

void D3DApp::Draw()
{
}

void D3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
}

void D3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
}

void D3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
}

void D3DApp::HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}

ID3D12Resource * D3DApp::GetCurrentBackBuffer() const
{
	return nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentBackBufferView() const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDepthStencilView() const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

void D3DApp::CalculateFrameStats()
{
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter * pAdapter)
{
}

void D3DApp::LogOutputDisplayNodes(IDXGIOutput * pOutput, DXGI_FORMAT format)
{
}

void D3DApp::Initialize()
{
}

void D3DApp::Run()
{
}

void D3DApp::InitMainWindow()
{
}

void D3DApp::InitDirect3D()
{
}

void D3DApp::CreateCommandObjects()
{
}

void D3DApp::CreateSwapChain()
{
}

void D3DApp::FlushCommandQueue()
{
}

