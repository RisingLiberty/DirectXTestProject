#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>

#include <string>
#include <unordered_map>

#include <DirectXMath.h>

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

UINT CalculateConstantBufferByteSize(UINT byteSize);

template<typename T>
static T Clamp(const T& x, const T& low, const T& high)
{
	return x < low ? low : (x > high ? high : x);
}

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& fileName, const D3D_SHADER_MACRO* defines, const std::string& entryPoint, const std::string& target);
Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& fileName);

// Defines a subrange of geometry in a MeshGeometry.
// This is for when multiple geometries are stored in one vertex and index buffer.
// It provides the offsets and data needed to draw a subset of geometry
// stores in the vertex and index buffers.
struct SubMeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh.
};

struct MeshGeometry
{
	// Give it a name so we can look it up by name
	std::string Name;

	// System memory copies. Use Blobs because the vertex/index format can be generic.
	// It is up to the cleint to cast appropriately.
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer
	// Use this container to define the Submesh geometries so we can draw the submeshes individually.
	std::unordered_map<std::string, SubMeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	// We can free this memory after we finish upload to the GPU
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)\
{\
    HRESULT _hr_ = (x);\
    if(FAILED(_hr_)) { throw DxException(_hr_, L#x, AnsiToWString(__FILE__), __LINE__); } \
}
#endif

#define MAT_4_IDENTITY DirectX::XMFLOAT4X4\
(\
	1.0f, 0.0f, 0.0f, 0.0f,\
	0.0f, 1.0f, 0.0f, 0.0f,\
	0.0f, 0.0f, 1.0f, 0.0f,\
	0.0f, 0.0f, 0.0f, 1.0f\
)

//#ifndef NUM_FRAME_RESOURCES
//#define NUM_FRAME_RESOURCES
//const int gNumFrameResources = 3;
//#endif

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MAT_4_IDENTITY;

	// Since lightning
	DirectX::XMFLOAT4X4 TexTransform = MAT_4_IDENTITY;

};

#define MaxLights 16

struct Light
{
	DirectX::XMFLOAT3 Strength; // Light color
	float FalloffStart;	// Points/Spot light only
	DirectX::XMFLOAT3 Direction; // Directional/Spot light only
	float FalloffEnd;	// Point/Spot light only
	DirectX::XMFLOAT3 Position; // Points/Spot light only
	float SpotPower; // Spot light only
};

// Contains constant data that is fixed over a given rendering pass.
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 InvView = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 Proj = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 InvProj = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 ViewProj = MAT_4_IDENTITY;
	DirectX::XMFLOAT4X4 InvViewProj = MAT_4_IDENTITY;
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	//Since lightning
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

// The idea of these 2 cbuffer is to group constant based on updated frequency.
// The per pass constant only need to be updated once per rendering pass.
// The object constants only need to change when an object's world matrix changes.
// If we had a static object in the scene, like a tree, we only need to set its world matrix
// once to a constant buffer and then never update the constant buffer again.

// Note: As a optimization freak, these optimization features can give me orgams!

//HLSL code:
/*
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
};

cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
};
*/

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct LightningVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

const int gNumFrameResources = 3;

struct Material
{
	// Unique material name for lookup
	std::string Name;

	// Index into constant buffer corresponding to this material
	int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	int DiffuseSrvHeapIndex = -1;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the update to each FrameResource.
	// Thus, when we modify a material, we should set NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Material constant buffer data used for shading
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	DirectX::XMFLOAT4X4 MatTransform = MAT_4_IDENTITY;

	// Note: shininess = 1 - roughness
};

// Lightweight structure stores parameters to draw a shape.
// This will vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space relative to the world space,
	// which defines the position, orientation and scale of the object in the world.
	DirectX::XMFLOAT4X4 World = MAT_4_IDENTITY;

	// Since lightning Demo
	DirectX::XMFLOAT4X4 TexTransform = MAT_4_IDENTITY;

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// because we have an object cbuffer for each FrameResource, we have to apply the update to each FrameResource.
	// Thus, when we modify object data we should set NumFramesDirty = s_NumFrameResources so that each frame resource get the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer coorespond to the ObjectCB for this render item
	UINT ObjCBIndex = -1;

	// Geometry associated with this render item
	// Note: multiple render items can share the same geometry.
	MeshGeometry* Geometry = nullptr;

	Material* Material = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };

	float Roughness = 0.25f;

	DirectX::XMFLOAT4X4 MatTransform = MAT_4_IDENTITY;
};

int Rand(int a, int b);
float RandF();
float RandF(float a, float b);
DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX m);