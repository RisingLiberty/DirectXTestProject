#include <iostream>

//#include "InitD3DApp.h"
//#include "DrawingD3DApp.h"
#include "2.2 DrawingD3DAppII/DrawingD3DAppII.h"
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
	
	DrawingD3DAppII app(hInstance);

	return app.Start();
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

