#include <d3d12.h>

#include <iostream>


//Because Direct X is a Windows-only api we can use Windows-only methods
class HelloDirectX
{
public:
	HelloDirectX()
	{
		Initialize();
	}

private:
	void Initialize()
	{
		InitializeD3D();
	}

	void InitializeD3D()
	{
		
	}
};

int main()
{
	try
	{

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}