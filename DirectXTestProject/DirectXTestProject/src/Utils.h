#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>

#include <string>


class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

	std::wstring ToString() const;

private:
	HRESULT m_ErrorCode = S_OK;
	std::wstring m_FunctionName;
	std::wstring m_FileName;
	int m_LineNr = -1;
};

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

//Because an intermediate upload buffer is required to initialize the data of a default buffer,
//we build the following utility function to avoid repeating this work every time we need a default buffer
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)\
{\
    HRESULT hr = (x);\
    if(FAILED(hr)) { throw DxException(hr, L#x, AnsiToWString(__FILE__), __LINE__); } \
}
#endif