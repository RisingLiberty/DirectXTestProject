#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

//Link necessary d3d12 librries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	~D3DApp();

	void Start();

	//Static window procedure method for handling window events
	static LRESULT CALLBACK WindowProcedureStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


	bool IsMultiSampling() const;
	float GetAspectRatio() const;
	HWND GetMainWindow() const;

	void IsMultiSampling(bool isMultiSampling);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const float dTime) = 0;
	virtual void Draw() = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

	virtual void HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	void CalculateFrameStats();
	void LogAdapterOutputs(IDXGIAdapter* pAdapter);
	void LogOutputDisplayNodes(IDXGIOutput* pOutput, DXGI_FORMAT format);

private:
	void Initialize();
	void Run();

	void InitMainWindow();
	void InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

private:
	HINSTANCE m_AppInstance;
	HWND m_WindowHandle;
	bool m_IsPaused;
	bool m_IsResizing;
	bool m_IsMultiSampling;
	UINT m_MultiSamplingQuality;

	GameTimer m_Timer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_DxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
	UINT64 m_CurrentFence;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_DirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

	static const int m_SwapchChainBufferCount = 2;
	int m_CurrentBackBuffer;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> m_SwapChainBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap;

	D3D12_VIEWPORT m_ScreenViewport;
	D3D12_RECT m_ScissorRect;

	UINT m_RtvDescriptorSize;
	UINT m_DsvDescriptorSize;
	UINT m_CbvSrvUavDescriptorSize;

	//Derived class should set these in derived constructor to customize starting values.
	std::string m_MainwndCaption;
	D3D_DRIVER_TYPE m_D3DDriverType;
	DXGI_FORMAT m_BackBufferFormat;
	DXGI_FORMAT m_DepthStencilFormat;
	const int WIDTH = 800;
	const int HEIGHT = 600;
};
