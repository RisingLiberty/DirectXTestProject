#pragma once

#include <Windows.h>

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

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)\
{\
    HRESULT hr = (x);\
    if(FAILED(hr)) { throw DxException(hr, L#x, AnsiToWString(__FILE__), __LINE__); } \
}
#endif