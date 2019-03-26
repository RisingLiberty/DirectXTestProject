#include "DrawingD3DAppIII.h"

#include "Waves.h"

#include "1.0 Core/GeometryGenerator.h"
#include "1.0 Core/FrameResource.h"

using namespace DirectX;

DrawingD3DAppIII::DrawingD3DAppIII(HINSTANCE hInstance) :
	D3DAppBase(hInstance)
{

}

HRESULT DrawingD3DAppIII::Initialize()
{
	ThrowIfFailed(D3DAppBase::Initialize());

	ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	this->BuildRootSignature();
	this->BuildShadersAndInputLayout();
	this->BuildLandGeometry();
	this->BuildWavesGeometryBuffers();
	//this->BuildRenderItems();
	this->BuildFrameResources();
	this->BuildPsos();

}

void DrawingD3DAppIII::Update(float dTime)
{

}

void DrawingD3DAppIII::Draw()
{
	// Bind per-pass constant buffer. We only need to do this once per-pass.
	ID3D12Resource* passCB = m_CurrentFrameResource->PassCB->GetResource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);
}

void DrawingD3DAppIII::BuildFrameResources()
{

}

void DrawingD3DAppIII::BuildRootSignature()
{
	// Root paramter can be a table, root descriptor or root constans.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// The following code specifies the shader register this paramter is bound to register b0 and b1)

	// Create root CBV.
	slotRootParameter[0].InitAsConstantBufferView(0); //Per-object CBV
	slotRootParameter[1].InitAsConstantBufferView(1); //Per-pass CBV

	//A root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//RootParndex: The index of the root paramtere we are bidning a CBV to.
	//BufferLocation: the virtual address to the resource that contains the constant buffer data.
	//ID3D12GRaphicsCommandList::SetGraphicsRootConstantBuffer(UINT RootParameterIndex, D3D12_GPU_VIRTUAL ADDRESS BufferLocation);
}

void DrawingD3DAppIII::BuildDescriptorHeaps()
{

}

void DrawingD3DAppIII::BuildPsos()
{

}

void DrawingD3DAppIII::BuildShadersAndInputLayout()
{

}

void DrawingD3DAppIII::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	// Extract the vertex elements we are interested and apply the height function to each vertex
	// In addition, color the vertices based on their height so we have sandy looking beaches, grassy low hills,
	// and snow mountain peaks.
	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		Vertex& vertex = vertices[i];;

		vertex.Pos = p;
		vertex.Pos.y = GetHillsHeight(p.x, p.z);
		
		// Color the vertex based on its height.
		if (vertex.Pos.y < -10.0f)
		{
			// Sandy beach color
			vertex.Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (vertex.Pos.y < 5.0f)
		{
			// Light yellow-green
			vertex.Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (vertex.Pos.y < 12.0f)
		{
			// Dark yellow-green
			vertex.Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertex.Pos.y < 20.0f)
		{
			// Dark brown
			vertex.Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow
			vertex.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<uint16_t> indices = grid.GetIndices16();

	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);
	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(Vertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	geometry->DrawArgs["grid"] = subMesh;

	m_Geometries["landGeo"] = std::move(geometry);
}

void DrawingD3DAppIII::BuildWavesGeometryBuffers()
{
	std::vector<uint16_t> indices(3 * m_Waves->GetTriangleCount()); // 3 indices per face
	assert(m_Waves->GetVertexCount() < 0x0000ffff);

	// Iterate over each quad
	int rowCount = m_Waves->GetRowCount();
	int columnCount = m_Waves->GetColumnCount();
	int k = 0;
	for (int row = 0; row < rowCount - 1; ++row)
	{
		for (int column = 0; column < columnCount - 1; ++column)
		{
			indices[k + 0] = row * rowCount + column;
			indices[k + 1] = row * rowCount + column + 1;
			indices[k + 2] = (row + 1) * rowCount + column;

			indices[k + 3] = (row + 1) * rowCount + column;
			indices[k + 4] = row * rowCount + column + 1;
			indices[k + 5] = (row + 1) * rowCount + column + 1;

			k += 6; // move to next quad
		}
	}

	UINT vbByteSize = m_Waves->GetVertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "waterGeo";

	// Set dynamically
	geometry->VertexBufferCPU = nullptr;
	geometry->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->IndexBufferGPU = CreateDefaultBuffer(m_pDevice.Get(), m_CommandList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexByteStride = sizeof(Vertex);
	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubMeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	geometry->DrawArgs["grid"] = subMesh;

	m_Geometries["waterGeo"] = std::move(geometry);
}

float DrawingD3DAppIII::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z*sinf(0.1f * x) + x * cosf(0.1f * z));
}

void DrawingD3DAppIII::DrawRenderItems(ID3D12GraphicsCommandList* pCommandList, const std::vector<RenderItem*>& renderItems)
{
	UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

	ID3D12Resource* objectCB = m_CurrentFrameResource->ObjectCB->GetResource();

	// For each render item..
	for (RenderItem* pRenderItem : renderItems)
	{
		pCommandList->IASetVertexBuffers(0, 1, &pRenderItem->Geometry->GetVertexBufferView());
		pCommandList->IASetIndexBuffer(&pRenderItem->Geometry->GetIndexBufferView());
		pCommandList->IASetPrimitiveTopology(pRenderItem->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += pRenderItem->ObjCBIndex * objCBByteSize;

		pCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		pCommandList->DrawIndexedInstanced(pRenderItem->IndexCount, 1, pRenderItem->StartIndexLocation, pRenderItem->BaseVertexLocation, pRenderItem->StartIndexLocation);
	}

}