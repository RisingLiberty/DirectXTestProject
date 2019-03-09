#include "Utils.h"

#include <sstream>
#include <comdef.h>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNr):
	m_ErrorCode(hr),
	m_FunctionName(functionName),
	m_FileName(fileName),
	m_LineNr(lineNr)
{
}

std::wstring DxException::ToString() const
{
	//Get the string description of the error code
	_com_error err(m_ErrorCode);
	std::wstring msg = err.ErrorMessage();

	std::wstringstream ss;

	ss << m_FunctionName << L" failed in " << m_FileName << L"on line " << m_LineNr << L"\n";
	ss << L"error: " << msg << L"\n";

	return ss.str();
}