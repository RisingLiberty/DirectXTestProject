#include <iostream>

#define DRAWING_D3D_3

#ifdef INITIALIZE_DIRECT_X
#include "1.1 InitD3D/InitD3DApp.h"
#endif

#ifdef DRAWING_D3D_1
#include "2.1 DrawingD3DApp/DrawingD3DApp.h"
#endif

#ifdef DRAWING_D3D_2
#include "2.2 DrawingD3DAppII/DrawingD3DAppII.h"
#endif

#ifdef DRAWING_D3D_3
#include "2.3 DrawingD3DAppIII/DrawingD3DAppIII.h"
#endif

#include "Utils.h"

#include <dxgidebug.h>
    
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd)
{
	//Notify user if heap is corrupt
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	//Enable runtime memory leak check for debug builds.
#if defined(DEBUG) || defined (_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	typedef HRESULT(__stdcall *fPtr)(const IID&, void**);
	HMODULE hd12 = LoadLibrary(L"dxgidebug.dll");
	fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hd12, "DXGIGetDebugInterface");

	IDXGIDebug* pDXGIDebug;
	DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDXGIDebug);
	//_CrtSetBreakAlloc(...);
#endif
	
	D3DApp* app = new DrawingD3DAppIII(hInstance);

	return app->Start();
}

int main()
{
	int err = 0;
	try
	{
		err = WinMain(GetModuleHandle(0), 0, 0, SW_SHOW);
	}
	catch (DxException& e)
	{
		std::wcerr << e.ToString() << std::endl;
		std::wcin.get();
		return 0;
	}

	return err;
}

