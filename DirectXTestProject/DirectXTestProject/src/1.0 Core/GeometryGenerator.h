#pragma once

#include <stdint.h>
#include <vector>

#include <DirectXMath.h>

class GeometryGenerator
{
public:
	
	struct Vertex
	{
		Vertex();
		Vertex
		(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT3& t,
			const DirectX::XMFLOAT2& uv
		);

		Vertex
		(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v
		);

		Vertex(const Vertex& other);
		Vertex(const Vertex&& other);

		Vertex& operator=(const Vertex& vertex);

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices32;

		std::vector<uint16_t>& GetIndices16();

	private:
		std::vector<uint16_t> m_Indices16;
	};
	
	MeshData CreateBox(float width, float height, float depth, uint32_t numSubdivisions);
	MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32_t m, uint32_t n);
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);
	MeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
	MeshData CreateGeoSphere(float radius, uint32_t numSubdivisions);

	void GeometryGenerator::Subdivide(MeshData& meshData);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, MeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, MeshData& meshData);

	Vertex MidPoint(const Vertex& v0, const Vertex& v1);

};