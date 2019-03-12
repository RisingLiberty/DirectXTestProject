#pragma once

//Define to minimize the number of header files included in windows.h
#define WIN32_LEAN_AND_MEAN

//Basic windows methods
#include <Windows.h>

//Contains the definition for the CommandLineToArgvW function
//This function wil be used later to parse the command-line arguments passed to the application
#include <shellapi.h> //For commandLineToArgvW

//The min/max macros confilct with like-named member function
//only use std::min and std::max defined in <algorithm>
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

//In order to define a function called CreateWindow, the windows
//macro needs to be undefined
#if defined(CreateWindow)
#undef CreateWindow
#endif

//Windows Runtime Library. Needed for Microsoft::WRL::Microsoft::WRL::ComPtr<> template class
//The Microsoft::WRL::ComPtr template class provides smart pointer functionality for COM objects.

#include <wrl.h>

//Windows extended methods
//#include <windowsx.h>
//#include <Xinput.h>

//DirectX 12
#include <d3d12.h>

//The Microsoft DirectX Graphics Infrastructure is used to manage the low level tasks
//such as enumerating GPU adapters, presenting the rendered image to the screen, and handling
//full-screen transitions, that are not necssarily part of the Direct X rendering API.
//DXGI 1.6 adds functionality in order to detect HDR displays.
//http://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx
//http://msdn.microsoft.com/en-us/library/windows/desktop/ee417025(v=vs.85).aspx
//https://msdn.microsoft.com/en-us/library/windows/desktop/mt427784%28v=vs.85%29.aspx
#include <dxgi1_6.h>

//This contains functions to compile HLSL code at runtime.
//It is recommended to compile HLSL shaders at compile time but for demonstration purposes, 
//it might be more convenient to allow runtime compilation of HLSL shaders.
//https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
#include <d3dcompiler.h>
#include <dxgidebug.h>

//DirectX math
//provides SIMD-friendly C++ types and functions for commonly used
//graphics related programming.
#include <DirectXMath.h>
//#include <DirectXColors.h>
//#include <DirectXPackedVector.h>
//#include <DirectXCollision.h>

//D3D12 extension library
//This is not required to work with DirectX 12 but provides some useful classes that will
//simplify some of the functions that will be used throughout this tutorial.
//the d3dx12.h header is not included as part of the Windows 10 SDK and needs to be downloaded from here:
//https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12
#include <DirectX/d3dx12.h>

#include <initguid.h>
#include <array>
#include <vector>

#include "GameTimer.h"

//Because Direct X is a Windows-only api we can use Windows-only methods
class D3DApp
{
public:
	D3DApp(HINSTANCE instance);
	~D3DApp();

	int Start();

protected:
	virtual HRESULT Initialize();
	virtual void Update(const float dTime) = 0;
	virtual void Draw() = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	ID3D12Resource* GetCurrentBackBuffer() const;

	void FlushCommandQueue();

	// methods to handle mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y)	{}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
	std::wstring m_AppName;

	GameTimer m_GameTimer;

	HINSTANCE m_hInstance;

	HWND m_WindowHandle;

	UINT m_RtvDescriptorSize;
	UINT m_DsvDescriptorSize;
	UINT m_CbvSrvDescriptorSize;
	UINT m_4xMsaaQuality;
	UINT m_CurrentFence;

	int m_CurrentBackBuffer = 0;

	bool m_4xMsaaEnabled = false;
	bool m_IsPaused = false;

	static const uint32_t m_SwapChainBufferCount = 2;

	const DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	const int WIDTH = 1024;
	const int HEIGHT = 720;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorsRect;

	//Microsoft recommends to use IDXGIFactory1 and IDXGIAdapter1 when using DirectX >= 11.0
	Microsoft::WRL::ComPtr<IDXGIFactory1> m_pDxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_DirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer;

	//We need to create the descriptor heaps to store the descriptors/views out application needs.
	//A descriptor heap is represented by the ID3D12DescriptorHeap interface.
	//A heap is created with the ID3D12Device::CreateDescriptorHeap method.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap; //Render taget views
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap; //Depth stencil view

	std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> m_Adapters;
	std::vector<Microsoft::WRL::ComPtr<IDXGIOutput>> m_Outputs;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, m_SwapChainBufferCount> m_SwapChainBuffers;

private:
	int Run();

	HRESULT InitializeAdapters();
	HRESULT InitializeOutputs();
	HRESULT InitializeDisplays();

	HRESULT InitializeWindow();
	HRESULT InitializeD3D();
	HRESULT CreateCommandObjects();
	HRESULT CreateSwapChain();
	HRESULT CreateRtvAndDsvDescriptorHeaps();
	   
	static LRESULT CALLBACK WindowProcdureStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT HandleEvent(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void CheckFeatureSupport(ID3D12Device* pDevice, D3D12_FEATURE feature, void* pFeatureSupportData, size_t featureSupportDataSize);
	void CalculateFrameStats();
	void OnResize();
};