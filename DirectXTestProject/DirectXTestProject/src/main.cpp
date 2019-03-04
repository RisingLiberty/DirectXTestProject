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

//nodeMask: Set to 0 for single GPU system. Otherwise, the node mask identifies the physical
//GPU this command list is associated with.
//HRESULT ID3D12Device::CreateCommandList(UINT nodeMask, D3D12_COMMAND_lIST_TYPE type, ID3D12CommandAllocator * pCommandAllocator, REFIID riid, void** ppCommandList);



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
		HRESULT hr = D3D12CreateDevice(m_Adapters.front().Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(m_pDevice.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
			return hr;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Width = WIDTH;
		swapChainDesc.BufferDesc.Height = HEIGHT;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		swapChainDesc.OutputWindow = m_WindowHandle;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;




		return hr;
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
		featureLevelsInfo.NumFeatureLevels = featureLevels.size();
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

