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

//Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class
//The ComPtr template class provides smart pointer functionality for COM objects.

#include <wrl.h>
using namespace Microsoft::WRL;

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

//Residency
//Make sure to visit this when make a bigger application!!
//https://msdn.microsoft.com/en-us/library/windows/desktop/mt186622%28v=vs.85%29.aspx

//Both methods take as first parameter the number of resources in the array
//The second parameter is the an array of ID3D12Pageable objects
//HRESULT ID3D12Device::MakeResident(UINT numObjects, ID3D12Pageable* const *ppObjects)
//HRESULT ID3D12Device::Evict(UINT numObjects, ID3D12Pageable* const *ppObjects)

//The command Queue and Command Lists
//The GPU has a command queue. The CPU submits commands to the queue through the Direct3D API
//using command lists. It is important to understand that once a set of commands have been submitted
//to the command queue, they are not immediately executed by the GPU. They sit in the queue
//until the GPU is ready to process them, as the GPU is likely busy processing previously
//inserted commands.

//If the command queue gets empty, the GPU will idle because it does not have any work
//to do, on the other hand, if the command queue gets too full, the cpu will at some point
//have to idle while the GPU catches up. Both of these situations are undesirable,
//for high performance applications like games, the goal is to keep both CPU and GPU
//busy to take full advantage of the hardware resources available.

//In Direct3D 12, the command queue is represented by the ID3D12CommandQueue interface.
//It is create by filling out a D3D12_COMMAND_QUEUE_DESC structure descrbibing the queue
//and then calling ID3D12Device::CreateCommandQueue

/*
ComPtr<ID3D12CommandQueue> m_CommandQueue;

...


D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
the IDD_PPV_ARGS macro is defined as:
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
Where __uuidof(**ppType)) evaluates to the COM interface ID of (**(ppType)),
which in the above code is ID3D12CommandQueue.
The IID_PPV_ARGS_Helper function essentially cast ppType to a void**. 
*/

//One of the primary methods of this interface is the ExecuteCommandLists method
//which adds the commands in the command list to the queue:

/*

//the first parameter is the number of commands lists in the array
//the second paramater is the first element in an array of command lists.
void Id3D12::CommandQueue::ExecuteCommandLists(UINT count, ID3D12CommandList* const *ppCommandLists);

*/

//The names these methods suggest that the commands are executed immediately, but they are not.
//The above code just adds commands to the command list. The ExecuteCommandLists method
//adds the commands to the command queue, and the GPU processes command from the queue.
//When we are done adding commands to the list we must call Id3D12GraphicsCommandList::Close
//The command list must be closed before passing it off to ID3D12CommandQueue::EXecuteCommandLists.

//Associated with a command list is a memory backing class called an ID3D12CommandAllocator
//As commands are recorded to the command list, they will actually be stored in the associated
//command allocator. When a command list is executed via ID3D12CommandQueue::ExecuteCommandLists,
//The command queue will reference the commands in the allocator. A command allocator is created from
//the ID3D12Devcie.

//type: tyhe type of command lists that can be associated with this allocator.
//D3D12_COMMAND_LIST_tYPE_DIRECT: Stores a list of commands to directly be executed by the GPU
//D3D12_COMMAND_LIST_TYPE_BUNDLE: Specifies the command list represents a bubdle.
//These is some CPU overhead in buildin a command list, so Direct3D 12 provides
//an optimization that allows us to record a sequence of commands into a so-called bundle.
//After a bundle has been recorded, the driver will preprocess the commands to optimize
//their execution during rendering.
//Therefore, bundles should be recorded at initialization time. The use of bundles
//should be thought of as an optimization to use if profiling shows building a particular
//command lists are taking significact time.
//The Direct3D 12 drawing API is already very efficient, so you should not need to use buddles often,
//and you should only use them if you can demonstrate a performance gain by them

//riid: The COM ID of the ID3D12CommandAllcocator interface we want to create.

//ppCommandAllocator: Outputs a pointer to the created command allocator.

//HRESULT ID3D12Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type, REFIID riid, void** ppCommandAllocator)

//--------------------------------------------------

//nodeMask: Set to 0 for single GPU system. Otherwise, the node mask identifies the physical
//GPU this command list is associated with.

//type: The type of command list: either D3D12_COMMAND_LIST_TYPE_DIRECT or D3D12_COMMAND_LIST_TYPE_BUNDLE

//pCommandAllocator: The allocator to be associated with the created command list.
//The command allocator type must match the command list type.

//pInitialState: Specifies the initial pipeline state of the command list. 
//This can be null for bundles, and in the special case where a command list is executed
//for initialization purposes and does not contain any draw commands.

//riid: The COM ID of the ID3D12CommandList interface we want to create.

//ppCommandList: Outputs a pointer to the create command list

//HRESULT ID3D12Device::CreateCommandList(UINT nodeMask, D3D12_COMMAND_lIST_TYPE type, ID3D12CommandAllocator * pCommandAllocator, REFIID riid, void** ppCommandList);

//You can use the ID3D12Device::GetNodeCount method to query the number of GPU adapter nodes on the system.

using namespace DirectX;

#include <initguid.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <cassert>
#include <array>


//Because Direct X is a Windows-only api we can use Windows-only methods
class HelloDirectX
{
public:
	HelloDirectX(HINSTANCE hInstance):
		m_hInstance(hInstance)
	{
		Initialize();
	}

	int MainLoop()
	{


		return 0;
	}

private:
	void Initialize()
	{
		InitializeAdapterAndOutput();
		//InitializeWindow();
		//InitializeD3D();
	}

	HRESULT InitializeAdapterAndOutput()
	{
		HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(m_pDxgiFactory.ReleaseAndGetAddressOf()));
		if (FAILED(hr)) return hr;

		int adapterNr = 0;
		ComPtr<IDXGIAdapter1> pAdapter = nullptr;
		while (m_pDxgiFactory->EnumAdapters1(adapterNr++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);

			std::wcout << L"Adapter: " << desc.Description << L"\n";

			m_Adapters.emplace_back(pAdapter);
		}

		for (auto& pAdapter : m_Adapters)
		{
			int outputNr = 0;
			ComPtr<IDXGIOutput> pTempOutput;
			while (pAdapter->EnumOutputs(outputNr++, pTempOutput.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
			{
				IDXGIOutput* pOutput;
				hr = pTempOutput->QueryInterface(__uuidof(IDXGIOutput), reinterpret_cast<void**>(&pOutput));

				if (SUCCEEDED(hr))
				{
					DXGI_OUTPUT_DESC desc;
					pOutput->GetDesc(&desc);

					std::wcout << "Output: " << desc.DeviceName << L"\n";

					m_Outputs.emplace_back(pOutput);
				}
				else
					return hr;
			}
		}

		for (auto& pOutput : m_Outputs)
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			UINT count = 0;
			UINT flags = 0;

			//Call with nullptr to get list count.
			pOutput->GetDisplayModeList(format, flags, &count, nullptr);

			std::vector<DXGI_MODE_DESC> modeList(count);
			pOutput->GetDisplayModeList(format, flags, &count, modeList.data());

			for (DXGI_MODE_DESC& mode : modeList)
			{
				UINT n = mode.RefreshRate.Numerator;
				UINT d = mode.RefreshRate.Denominator;

				std::wcout << L"Width = " << mode.Width << L"\n";
				std::wcout << L"Height = " << mode.Height << L"\n";
				std::wcout << L"Refresh = " << n << L"/" << d << " = " << (float)n/d << L"\n";
			}
		}

		return hr;
	}

	HRESULT InitializeWindow()
	{
		//Create window class
		const std::string windowClassName = "DirectXTestProjectWindowClass";

		WNDCLASS windowClass = {};
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hIcon = nullptr;
		windowClass.hbrBackground = nullptr;
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProcedureStatic;
		windowClass.hInstance = m_hInstance;
		windowClass.lpszClassName = windowClassName.c_str();

		if (!RegisterClass(&windowClass))
		{
			DWORD err = GetLastError();
			return HRESULT_FROM_WIN32(err);
		}

		//Create window
		DXGI_OUTPUT_DESC outputDesc;
		HRESULT hr = m_Outputs.front()->GetDesc(&outputDesc);
		if (FAILED(hr)) return hr;

		RECT r = { 0, 0, WIDTH, HEIGHT };
		AdjustWindowRect(&r, WS_OVERLAPPED, false);
		int winWidth = r.right - r.left;
		int winHeight = r.bottom - r.top;

		int x = outputDesc.DesktopCoordinates.left + ((outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left));
		int y = (int)((outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top) * 0.5f - winHeight * 0.5f);

		//Now let's assign the window handle
		m_WindowHandle = CreateWindowA(
			windowClassName.c_str(), 
			windowClassName.c_str(),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
			x,
			y,
			winWidth,
			winHeight,
			NULL,
			nullptr,
			m_hInstance,
			this);

		if (!m_WindowHandle)
		{
			DWORD err = GetLastError();
			return HRESULT_FROM_WIN32(err);
		}

		ShowWindow(m_WindowHandle, SW_SHOWDEFAULT);

		return S_OK;
	}

	HRESULT InitializeD3D()
	{
		//Initialzing Direct3D begins by creating the Direct3D device. The device represents a display adapter.
		//Usually, the display adapter is a physical piece of 3D hardware(eg. graphics card) however, a systemc an also have a software
		//display adapter that emulates 3D hardware functionality(eg. the WARP adapter). The Direct3D 12 device is used to check feature
		//support, and create all other Direct3D interface objects like resourcees, views and command lists. The device can be created with the following function:
		//pAdapter: specifies the display adapter we want the created device to represent.
		//Specifyinh null fof this parameter uses the primary display adapter. 

		//Enable debug layer
#if defined(DEBUG) || defined (_DEBUG)
		ComPtr<ID3D12Debug> debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()));
		debugController->EnableDebugLayer();
#endif

		//pAdapter: Specifies the display adapter we want the created device to represent.
		//Specifying null for this parameter uses the primary display adapter.

		//MinimumFeatureLevel: the minimum feature level our application requires support for.
		//device creation will fail if the adapter does not support this feature level.
		
		//riid: the COM ID of the ID3D12Device interface we want to create.

		//ppDevice: returns the created device.
		HRESULT hr = D3D12CreateDevice(m_Adapters.front().Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));

		/*
		Fall back to warp adapter
		if (FAILED(hr))
		{
			//To enumerate over WARP adapters we need to make an IDXGIFactory4 object
			ComPtr<IDXGIFactory4> pFactory;
			CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf())

			ComPtr<IDXGIAdapter> pWarpAdapter;
			m_dxgiFactory->EnumWarmAdapter(IID_PPV_ARGS(pWarpAdapter.GetAddressOf());

			hr = D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()));
		}
		*/

		if (FAILED(hr))
			return hr;

		//Create the fence and descriptor sizes
		//After we have created our device, we can create our object for CPU/GPU synchronizatino.
		//In addtion, once we get to working with descriptors, we are going to need to know their size.
		//Descriptor sizes can vary across GPUs so we need to query this informatino. We cache the descriptor
		//sizes so that it is available when we need it for various descriptor types.
		hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));
		
		if (FAILED(hr))
			return hr;

		m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_DsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_CbvSrvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//Check 4x MSAA Quality Support
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		msQualityLevels.Format = m_BackBufferFormat;
		msQualityLevels.SampleCount = 4;
		msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLevels.NumQualityLevels = 0;

		hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));

		if (FAILED(hr))
			return hr;

		m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
		assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level");

		//Create command queue and command list.
		CreateCommandObjects();

		//Create swapchain
		CreateSwapChain();

		//Create descriptor heaps
		CreateRtvAndDsvDescriptorHeaps();

		//Create render target view
		//First get buffer resources that are stored in the swapchain
		//Buffer: an index identifying the particular back buffer we want to get (in case there's more than one.)
		//riid: The COM ID of the ID#D12Resource interface we want to obtain a pointer to
		//ppSurface: Returns a pointer to an ID3D12Resource that represents the back buffer.
		//HRESULT IDXGISwapChain::GetBuffer(UINT buffer, REFIID riid, void** ppSurface);

		//To create the render target view, we use the ID3D12Device::CreateRenderTargetView method:
		//ID3D12Resource* pResource: Specifies the resource that will be used as the render target.
		//pDesc: A pointer to a D3D12_RENDER_TARGET_VIEW_DESC. ammong other things, this structure describes the
		//data type (format) of the elements in the resource.
		//If the resource was created with a typed format, then this parameter can be null, which indicates
		//to create a view to the first mipmap level of the resource (back buffer only has 1) with the format the
		//resource was created with.
		//DestDescriptor: Handle to the descriptor that will store the created render target view
		//ID3D12Device::CreateRenderTargetView(ID3D12Resource* pResource, const D3D12_RENDER_TARGET_VIEW_DESC *pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DescDescriptor);
		
		

		return hr;
	}

	HRESULT CreateCommandObjects()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		
		HRESULT hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;

		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_DirectCmdListAlloc.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;

		//We specify null for the pipeline state object parameter.
		hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_DirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(m_CommandList.ReleaseAndGetAddressOf()));
		
		if (FAILED(hr))
			return hr;

		//Start off in a closed state.
		//This is because the first time we refer to the command list we will reset it, 
		//and it needs to be closed before calling Reset.
		m_CommandList->Close();
	}

	HRESULT CreateSwapChain()
	{
		//Release the old swap chain before creating a new one
		m_pSwapChain.Reset();


		//Buffer desc: This structure describes the properties of the back buffer we want to create
		//The main properties we are concerned with are width, height and pixel format.

		//SampleDesc: The number of multisamples and quality level.

		//BufferUsage: specify DXGI_USAGE_RENDER_TARGET_OUTPUT since we are going to be rendering to the back buffer.

		//BufferCount: the number of buffers to use in the swap chain. specify 2 for double buffering.

		//OutputWindow: a handle to the window we are rendering into

		//Windowed: specify true to run in windowed mode or false for full screen mode.

		//SwapEffect: specify DXGI_SWAP_EFFECT_FLIP_DISCARD.

		//Flags: optional flags. if you specify DXGI_SWAP_CHAIN_FLAH_ALLOW_MODE_SWITCH, 
		//then when the applicatoin is switching to full-screen mode, it will choose a display mode that best
		//matches the current application window dimensions. If this flag is not specified, then when the application
		//is switching to full-screen mode, it will use the current desktip display mode
		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = WIDTH;
		sd.BufferDesc.Height = HEIGHT;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = m_BackBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = m_4xMsaaEnabled ? 4 : 1;
		sd.SampleDesc.Quality = m_4xMsaaEnabled? (m_4xMsaaQuality - 1) : 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // we're rendering to this buffer
		sd.BufferCount = m_SwapChainBufferCount; //Double buffering
		sd.OutputWindow = m_WindowHandle;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		//Note: swap chain uses queue to perform flush
		
		HRESULT hr = m_pDxgiFactory->CreateSwapChain(m_pCommandQueue.Get(), &sd, m_pSwapChain.ReleaseAndGetAddressOf());
		
		return hr;
	}

	HRESULT CreateRtvAndDsvDescriptorHeaps()
	{
		HRESULT hr = S_OK;

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = m_SwapChainBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;


		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;

		//After we create the heaps, we need to be able to access the descriptors they store.
		//Our application references descriptor through handles.
		//A handle to the first descriptor in a heap is obtained with the ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart method.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < m_SwapChainBufferCount; ++i)
		{
			//Get the ith buffer in the swapchain
			HRESULT hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_SwapChainBuffers[i].GetAddressOf()));

			if (FAILED(hr))
				return hr;

			//Create RTV to it
			m_pDevice->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);

			//Next entry in heap
			rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
		}

		//We now need to create the depth/stencil buffer.
		//D3D12_RESOURCE_DESC:
		//{
		//	Dimension: Dimension of the resource, which is one of the following enunmerated types:
		//	D3D12_RESOURCE_DIMENSTION_UNKOWN = 0
		//	D3D12_RESOURCE_DIMENSTION_BUFFER = 1
		//	D3D12_RESOURCE_DIMENSTION_TEXTURE1D = 2
		//	D3D12_rESOURCE_DIMENSTION_TEXTURE2D = 3
		//	D3D12_RESOURCE_DIMENSTION_TEXTURE3D = 4
		//
		//	Width: the width of the texture in texels, for buffer resources, this is the number of bytes in the buffer.
		//	Height: the height of the texture in texels.
		//	DepthOrArraySize: the depth of the texture in texels, or the texture array size (for 1D and 2D textures).
		//	Note that you cannot have a texture array of 3D textures.
		//
		//	MipLevels: the number of mipmap levels.
		//	Format: A member of the DXGI_FORMAT enumerated type specifying the format of the texels.
		//	SampleDesc: The number of multisamples and quality level. Recall that 4X MSAA uses a back buffer
		//	and depth buffer 4X bigger than the screen resolution, in order to store color and depth/stencil information
		//	per subpixel. Therefore, the multisampling settings used for the depth/stencil buffer must match the settings used
		//	for the render target.
		//
		//	Layout: a member of the D3D12_TEXTURE_LAYOUT enumerated type that specifies the texture layout.
		//	for now, we do not have to worry about the layout and can specify D3D12TEXTURE_LAYOUT_UNKNOWN.
		//
		//	MiscFlags: Miscellaneos resource flags. For a depth/stencil buffer,
		//	specify D3D12_RESOURCE_MISC_DEPTH_STENCIL
		//}

		//GPU resources live in heaps, which are essentially block of GPU memory with certain properties.
		//The ID3D12Device::CreateCommittedResource method creates and commits a resource to a particular heap
		//with the properties we specify.

		//pHeapProperties: The properties of the heap we want to commit the resource to.
		//Some of these properties are for advanced usage.
		//D3D12_HEAP_TYPE
		//{
		//	D3D12_HEAP_TYPE_DEFAULT: Default heap. This is where we commit resources where we need to upload
		//data from the CPU to the GPU resource.
		//	D3D12_HEAP_TYPE_UPLOAD: Upload heap. This is where we commit resources where we need to upload data from CPU to the GPU resource.
		//	D3D12_HEA_TYPE_READBACK: Read-back heap. this is where we commit resources that need to be read by the CPU.
		//	D3D12_HEAP_TYPE_CUSTOM: For advanced usage scenarios --see the MSDN documentation for more information.
		//}

		//HeapMiscFlags: Additional flags about the heap we want to commit the resource to. this will usually be D3D12_HEAP_MISC_NONE
		//pResourceDesc: Pointer to a D3D12_RESOURCE_DESC instance describing the resource we want to create.

		//pResourceDesc: Pointer to a D3D12_RESOURCE_DESC instance describing the resource we want to create.

		//InitialResourceState: Set the initial state of the resource when it is created.
		//For the depth/stencil buffer, the initial state will be D3D12_RESOURCE_USAGE_INITIAL, and then we will
		//want to transition it to the D3D12_RESOURCE_USAGE_DEPTH so it can be bound to the pipeline as a depth/stencil view.

		//pOptimizedClearValue: Pointer to a D3D12_CLEAR_VALUE object that describes an optimized value for clearing resources.
		//Clear calls that match the optimized clear value can potentially be faster than clear calls that do not match
		//the optimized clear value. Null can also be specified for this value to not specify an optimized clear value.
		//D3D12_CLEAR_VALUE
		//{
		//	DXGI_FORMAT format;
		//	union
		//	{
		//		FLOAT Color[4];
		//		D3D12_DEPTH_STENCIL_VALUE DepthStencil;
		//	};
		//};
		//riidResource: The COM ID of the ID3D12Resource interface we want to obtain a pointer to.
		//ppvResource: Return pointer to an ID3D12Resource that represents the newly created resource.
		//Note: Resources should be placed in the default heap for optimal performance.
		//Only use upload or read back heaps if you need those features.

		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = WIDTH;
		depthStencilDesc.Height = HEIGHT;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = m_DepthStencilFormat;
		depthStencilDesc.SampleDesc.Count = m_4xMsaaEnabled ? 4 : 1;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaEnabled ? (m_4xMsaaQuality - 1) : 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = m_DepthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		hr = m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(m_DepthStencilBuffer.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;

		//Create descriptor to mip level 0 of entire resource using the format of the resource.
		m_pDevice->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, GetDepthStencilView());

		//Transition the resource from its initial state to be used as a depth buffer
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		//note that we use the CD3DX12_HEAP_PROPERTIES helper constructor to create the heap properties strucutre,
		//which is implemented like so:
		//explicit cd3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE type, UINT creationNodeMask = 1, UINT nodeMask = 1)
		//{
		//	Type type;
		//	CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		//	MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//	CreationnodeMask = creationNodeMask
		// VisiblenodeMask = nodeMask
		//}

		//The second parameter of CreateDepthStencilView is a pointer to a D3D12_DEPTH_STENCIL_VIEW_DESC.
		//Among other things, this structure describes the data type (format) of the elements in the resource.
		//If the resource was created with a typed format, then this parameter can be null.
		
		//Setting the viewport
		//struct D3D12_VIEWPORT
		//{
		//	FLOAT TopLeftX;
		//	FLOAT TopLeftY;
		//	FLOAT Width;
		//	FLOAT Height;
		//	FLOAT MinDepth;
		//	FLOAT MaxDepth;
		//};

		//the first four data members define the viewport rectangle relative to the back buffer.
		//In Direct3D, depth values are stored in the depth buffer in a normalized range of 0 to -1.
		//Being able to transform the depth range can be used to achieve certain effects.
		//for example, you could set MinDepth = 0 and MaxDepth - 0, so that all objects drawn with this viewport
		//will have depth values of 0 and appear in front of all other objects in the scene.
		//However, usually MinDepth is set to 0 and MaxDepth set to 1 so that the depth values are not modified.
		D3D12_VIEWPORT vp;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(WIDTH);
		vp.Height = static_cast<float>(HEIGHT);
		vp.MinDepth = 0;
		vp.MaxDepth = 1;

		m_CommandList->RSSetViewports(1, &vp);
		//Note: you cannot specify multiple viewports to the same render target.
		//Multiple viewports are used for advanced techniques that render to multiple render targets at the same time.

		//Note: the viewport needs to be reset whenever the command list is reset.

		//You could use the viewport to implement split screens for 2 player game modes, for example.
		//You would create 2 viewports, one for the top and one for the bottom screen.
		//Then you would draw the 3D scene from the perspective of player 1 into the top part of the screen
		//and the perspective of player 2 in the bottom part of the screen.

		//Set the scissor rectangles
		//We cand fine a scissor rectangle relative to the back buffer such that pixels outside this rectangle are culled.
		//This can be used for optimizations. For example, if we know an area of the screen will contain a rectangular UI
		//element on top of everything, we do not need to process the pixels of the 3D world that the UI element would obscure.

		//A scissor rectangle is defined by a D3D12_RECT structure which is typedefed to the following structure:
		//typedef struct tagRECT
		//{
		//	LONG left
		//	LONG top
		//	LONG right
		//	LONG bottom
		//} RECT;

		//we set the scissor rectangle with Direct3D with the ID3D12CommandList::RSSetScissorRects method
		m_ScissorsRect = { 0, 0, WIDTH / 2, HEIGHT / 2 };
		m_CommandList->RSSetScissorRects(1, &m_ScissorsRect);
		
		//Note: you cannot specify multiple scissor rectangles on the same render target.
		//Multiple scissor rectangle are used for advanced techniques that render to multiple
		//render targets at the same time.
		
		//Note: The scissors rectangles need to be reset whenever the command list is reset.

		//Timing and animation
		
		//The perfromance timer measure time in units called counts. We obtain the current time value,
		//measured in counts, of the performance timer with the QueryPerformanceCounter function.
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		//To get the frequency (counts per second) of the performance timer, we use the QueryPerformanceFrequency function
		__int64 countsPerSec;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

		m_SecondsPerCount = 1.0 / (double)countsPerSec;

		//So in the future to get the time elapsed:
		//valueInSecs = valueInCounts * m_SecondsPerCount;

		/*
		MSDN has the following remark about QueryPerformanceCounter:
		“On a multiprocessor computer, it should not matter which processor is called.
		However, you can get different results on different processors due to bugs in the
		basic input/output system (BIOS) or the hardware abstraction layer (HAL).” You
		can use the SetThreadAffinityMask function so that the main application
		thread does not get switch to another processor.
		*/
		
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const
	{
		//CD3DX12 constructor to offset to the RTV of the current backbuffer
		//First parameter: Handle start
		//Second parameter: Index to offset
		//Third parameter: byte size of descriptor
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBuffer, m_RtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
	{
		return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	static LRESULT CALLBACK WindowProcedureStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	//Possible features we want to check the support of:
	//D3D12_FEATURE_D3D12_OPTIONS: Checks support for various Direct3D 12 features
	//D3D12_FEATURE_ARCHITECTURE: Checks support for hardware architecture features.
	//D3D12_FEATURE_FEATURE_LEVELS: Checks feature level support
	//D3D12_FEATURE_FORMAT_SUPPORT: Checks feature support for a given texture format.
	//(e.g. can the fromat be used as a render target, can the format be used with blending).
	//D3D12_FEATURE_MULTSAMPLE_QUALITY_LEVELS: Checks multisampling feature support.

	//pFeatureSupportData: pointer to a data structure to retrieve the feature support information.
	//The type of strucuture you use depends on what you specified for the Feature parameter.
	//D3D12_FEATURE_D3D12_OPTIONS --> D3D12_FEATURE_dATA_D3D12_OPTIONS
	//D3D12_FEATURE_ARCHITECTURE --> D3D12_FEATURE_DATA_ARCHITECTURE
	//D3D12_FEATURE_FEATURE_LEVELS --> D3D12_FEATURE_DATA_FEATURE_LEVELS
	//D3D12_FEATURE_FORMAT_SUPPORT --> D3D12_FEATURE_DATA_FROMAT_SUPPORT
	//D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS --> D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
	void CheckFeatureSupport(ID3D12Device* pDevice, D3D12_FEATURE feature, void* pFeatureSupportData, size_t featureSupportDataSize)
	{
		HRESULT hr;

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multiSampleQualityLevels;
		multiSampleQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //Back buffer format
		multiSampleQualityLevels.SampleCount = 4;
		multiSampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		multiSampleQualityLevels.NumQualityLevels = 0;

		//The second parameter is both an input and output parameter. For the input,
		//we must specify the texture format, sample count and flag we want to query multisampling
		//support for. The function will then fill out the quality level as the output.
		//Valid quality levels for a texture format and sample count combination range from zero
		//to NumQualityLevels-1.

		//The maximum number of samples that can be taken per pixel is defined by:
		//#define D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT (32)
		//However a sample count of 4 or 8 is common  in order to keep the performance and
		//memory cost of multisampling reasonable. If you do not wish to use multisampling,
		//set the sample count to 1 and the quality level to 0. All Direct3D 11 capable devices
		//support 4X multisampling for all render target formats.

		//Note: a DXGI_SAMPLE_DESC structure needs to be filled out for both the swapchain buffers
		//and the depth buffer. Both the back buffer and depth buffer must be created with the 
		//same multisampling settings.

				//Query the number of quality levels for a given texture format and sample count
		//using ID3D12Device::CheckFeatureSupport.

		//Direct3D 11 introduces the concept of feature levels, which roughly correspond to various
		//Direct3D version from version 9 to 11:
		//D3D_FEATURE_LEVEL_9_1
		//D3D_FEATURE_LEVEL_9_2
		//D3D_FEATURE_LEVEL_9_3
		//D3D_FEATURE_LEVEL_10_0
		//D3D_FEATURE_LEVEL_10_1
		//D3D_FEATURE_LEVEL_11_0
		//D3D_FEATURE_LEVEL_11_1
		//D3D_FEATURE_LEVEL_12_0
		//D3D_FEATURE_LEVEL_12_1

		//Feature levels make development easier, once you know the supported feature set,
		//you know the Direct3D functionality you have at your disposal

		hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multiSampleQualityLevels, sizeof(multiSampleQualityLevels));

		//Check feature levels
		std::array<D3D_FEATURE_LEVEL, 3> featureLevels =
		{
			D3D_FEATURE_LEVEL_12_0, //First check D3D 12
			D3D_FEATURE_LEVEL_11_0, //next, check D3D 11
			D3D_FEATURE_LEVEL_10_0  //finally, check D3D 10
		};

		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsInfo;
		featureLevelsInfo.NumFeatureLevels = static_cast<UINT>(featureLevels.size());
		featureLevelsInfo.pFeatureLevelsRequested = featureLevels.data();

		//For the input we specify the number of elements in a feature level array, and a pointer
		//to a feature level array which contains a list of feature levels we want to check hardware support for.
		//The functoin outputs the maximum suppoerted feature level through the MaxSupportedFeatureLevel field.
		hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsInfo, sizeof(featureLevelsInfo));

	}

private:
	HINSTANCE m_hInstance;
	
	//Microsoft recommends to use IDXGIFactory1 and IDXGIAdapter1 when using DirectX >= 11.0
	ComPtr<IDXGIFactory1> m_pDxgiFactory;
	std::vector<ComPtr<IDXGIAdapter1>> m_Adapters;
	std::vector<ComPtr<IDXGIOutput>> m_Outputs;

	//std::unique_ptr<IDXGIFactory1, std::function<void(IDXGIFactory1*)>> m_pDxgiFactory;
	//std::vector<std::unique_ptr<IDXGIAdapter1, std::function<void(IDXGIAdapter1*)>>> m_Adapters;
	//std::vector<std::unique_ptr<IDXGIOutput, std::function<void(IDXGIOutput*)>>> m_Outputs;

	HWND m_WindowHandle;
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12Fence> m_pFence;
	UINT m_RtvDescriptorSize;
	UINT m_DsvDescriptorSize;
	UINT m_CbvSrvDescriptorSize;
	UINT m_4xMsaaQuality;
	bool m_4xMsaaEnabled = false;
	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<ID3D12CommandAllocator> m_DirectCmdListAlloc;
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	static const uint32_t m_SwapChainBufferCount = 2;
	int m_CurrentBackBuffer = 0;
	//We need to create the descriptor heaps to store the descriptors/views out application needs.
	//A descriptor heap is represented by the ID3D12DescriptorHeap interface.
	//A heap is created with the ID3D12Device::CreateDescriptorHeap method.
	ComPtr<ID3D12DescriptorHeap> m_RtvHeap; //Render taget views
	ComPtr<ID3D12DescriptorHeap> m_DsvHeap; //Depth stencil view
	std::array<ComPtr<ID3D12Resource>, m_SwapChainBufferCount> m_SwapChainBuffers;
	ComPtr<ID3D12Resource> m_DepthStencilBuffer;
	D3D12_RECT m_ScissorsRect;
	double m_SecondsPerCount;




	const int WIDTH = 1024;
	const int HEIGHT = 720;






};
    
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd)
{
	//Notify user if heap is corrupt
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	//Enable runtime memory leak check for debug builds.
#if defined(DEBUG) || defined (_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	typedef HRESULT(__stdcall *fPtr)(const IID&, void**);
	HMODULE hd12 = LoadLibrary("dxgidebug.dll");
	fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hd12, "DXGIGetDebugInterface");

	IDXGIDebug* pDXGIDebug;
	DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDXGIDebug);
	//_CrtSetBreakAlloc(...);
#endif
	
	HelloDirectX app(hInstance);
	return app.MainLoop();
}

int main()
{
	int err = 0;
	try
	{
		err = WinMain(GetModuleHandle(0), 0, 0, SW_SHOW);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	std::cin.get();
	return err;
}

