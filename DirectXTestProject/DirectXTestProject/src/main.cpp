#include <iostream>

#include "D3DApp.h"
    
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
	
	D3DApp app(hInstance);
	app.Start();

	return 0;
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
		std::wcerr << e.what() << std::endl;
	}

	std::wcin.get();
	return err;
}

