#include "DrawingD3DApp.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "Utils.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex1
{
	XMFLOAT3 Pos;
	XMFLOAT3 Color;
};

DrawingD3DApp::DrawingD3DApp(HINSTANCE hInstance):
	D3DApp(hInstance)
{
	m_AppName = L"Drawing D3D App";
}

HRESULT DrawingD3DApp::Initialize()
{
	ThrowIfFailed(D3DApp::Initialize());

	//Input Layout
	//SemanticName: A string to associate with the element. This can be any valid variable name.
	//Semantics are used to map elements in the vertex strucutre to elements in the vertex shader input signature.
	//SemanticIndex: an index to attach to a semantic. A vertex strcuture may have more than one set of texture
	//coordinates, so rather than introducing a new semantic name, we can just attach an index to the end to distinguish
	//the two texture coordinate sets. A semantic with no index specified in the shader code defaults to index zero.
	//For instance: POSITION is equivalent to POSITION0
	//Format: A member of DXGI_FORMAT enumerated type specifying the format of this vertex element to Direct3D.
	//Common examples of formats:
	//	DXGI_FORMAT_R32_FLOAT			1D 32-bit float
	//	DXGI_FORMAT_R32G32_FLOAT		2d 32-bit float
	//	DXGI_FORMAT_R32G32B32_FLOAT		3d 32-bit float
	//	DXGI_FORMAT_R32G32B32A32_FLOAT	4d 32-bit float
	//  DXGI_FORMAT_R8_UINT				1d 8-bit unsigned int
	//	DXGI_FORMAT_R16G16_SINT			2d 16-bit signed int
	//	DXGI_FORMAT_R32G32B32_UINT		3d 32-bit unsigned int
	//  DXGI_FORMAT_R8G8B8A8_SINT		4d 8-bit signed int
	//	DXGI_FORMAT_R8G8B8A8_UINT		4d 8-bit unsigned int
	//Input slot: Specifies the input slot index the element will come from.
	//Direct3D supports 16 input slots (indexed from 0 -15) through which you can feed vertex data.
	//AlignedByteOffset: The offset, in bytes, from the start of the C++ vertex structure of the specified
	//input slot to start of the vertex component. 
	//For example, in the following vertex strucutre, the element Pos has a 0-byte offset, since its start coincides
	//with the start of the vertex structure. The element Normal has a 12-byte offset because we have to skip over
	//the bytes of Posto get to the start of Normal. The element Tex0 has 24-byte offset because we need to skip
	//over the bytes of Pos and Normal to get to the start of Tex0. the element Tex1 has a 32-byte offset because
	//we need to skip over the bytes of Pos, normal and Tex0 to get tot he start of Tex1
	//struct Vertex2
	//{
	//	XMFLOAT3 Pos;		// 0-byte offset
	//	XMFLOAT3 Normal;	// 12-byte offset
	//  XMFLOAT2 Tex0;		// 24-byte offset
	//  XMFLOAT2 Tex1;		// 32-byte offset
	//};
	//InputSlotClass: specify D3D12_INPUT_PER_VERTEX_DATA.
	//InstanceDataStepRate: Specify 0

	m_InputLayout = 
	{{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	}};

	//width: refers to the number of bytes in the buffer.
	//CD3DX12_RESOURCE_DESC::buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0)

	//Note: CD3DX12_RESOURCE_DESC class also provides convenience methods for constructing a D3D12_RESOURCE_DESC 
	//that describes texture resources and querying information about the resource:

	//CD3DX12_RESOURCE_DESC::Tex1D
	//CD3DX12_RESOURCE_DESC::Tex2D
	//CD3DX12_RESOURCE_DESC::Tex3D

	return S_OK;
}