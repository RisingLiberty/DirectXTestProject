#include "GeometryGenerator.h"

#include <algorithm>

using namespace DirectX;

GeometryGenerator::Vertex::Vertex()
{
}

GeometryGenerator::Vertex::Vertex
(
	const XMFLOAT3& p,
	const XMFLOAT3& n,
	const XMFLOAT3& t,
	const XMFLOAT2& uv
) :
	Position(p),
	Normal(n),
	TangentU(t),
	TexC(uv)
{

}

GeometryGenerator::Vertex::Vertex
(
	float px, float py, float pz,
	float nx, float ny, float nz,
	float tx, float ty, float tz,
	float u, float v
) :
	Position(px, py, pz),
	Normal(nx, ny, nz),
	TangentU(tx, ty, tz),
	TexC(u, v)
{

}

GeometryGenerator::Vertex::Vertex(const GeometryGenerator::Vertex& vertex) :
	Position(vertex.Position),
	Normal(vertex.Normal),
	TangentU(vertex.TangentU),
	TexC(vertex.TexC)
{

}


GeometryGenerator::Vertex::Vertex(const GeometryGenerator::Vertex&& vertex) :
	Position(vertex.Position),
	Normal(vertex.Normal),
	TangentU(vertex.TangentU),
	TexC(vertex.TexC)
{

}

GeometryGenerator::Vertex& GeometryGenerator::Vertex::operator=(const GeometryGenerator::Vertex& vertex)
{
	Position = vertex.Position;
	Normal = vertex.Normal;
	TangentU = vertex.TangentU;
	TexC = vertex.TexC;

	return *this;
}


std::vector<uint16_t>& GeometryGenerator::MeshData::GetIndices16()
{
	if (m_Indices16.empty())
	{
		m_Indices16.resize(Indices32.size());
		for (size_t i = 0; i < Indices32.size(); ++i)
			m_Indices16[i] = static_cast<uint16_t>(Indices32[i]);
	}

	return m_Indices16;
}

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	MeshData meshData;

	// Create the vertices

	Vertex v[24];

	float haldWidth = 0.5f * width;
	float halfHeight = 0.5f * height;
	float halfDepth = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-haldWidth, -halfHeight, -halfDepth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-haldWidth, +halfHeight, -halfDepth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+haldWidth, +halfHeight, -halfDepth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+haldWidth, -halfHeight, -halfDepth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-haldWidth, -halfHeight, +halfDepth, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+haldWidth, -halfHeight, +halfDepth, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+haldWidth, +halfHeight, +halfDepth, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-haldWidth, +halfHeight, +halfDepth, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = Vertex(-haldWidth, +halfHeight, -halfDepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-haldWidth, +halfHeight, +halfDepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+haldWidth, +halfHeight, +halfDepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+haldWidth, +halfHeight, -halfDepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-haldWidth, -halfHeight, -halfDepth, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+haldWidth, -halfHeight, -halfDepth, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+haldWidth, -halfHeight, +halfDepth, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-haldWidth, -halfHeight, +halfDepth, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-haldWidth, -halfHeight, +halfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-haldWidth, +halfHeight, +halfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-haldWidth, +halfHeight, -halfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-haldWidth, -halfHeight, -halfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+haldWidth, -halfHeight, -halfDepth, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+haldWidth, +halfHeight, -halfDepth, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+haldWidth, +halfHeight, +halfDepth, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+haldWidth, -halfHeight, +halfDepth, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices32.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);

	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	MeshData meshData;

	uint32_t vertexCount = m * n;
	uint32_t faceCount = (m - 1)*(n - 1) * 2;

	// Create the vertices

	float halfWidth = 0.5f * width;
	float halfDepth = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.Vertices.resize(vertexCount);
	for (uint32_t i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.Vertices[i*n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i*n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i*n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// stretch texture over grid.
			meshData.Vertices[i*n + j].TexC.x = j * du;
			meshData.Vertices[i*n + j].TexC.y = i * dv;
		}
	}

	// Create the indices

	// 3 indices per face
	meshData.Indices32.resize(faceCount * 3);

	// Iterate over each quad and copmute indices.
	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i)
	{
		for (uint32_t j = 0; j < n - 1; ++j)
		{
			meshData.Indices32[k] = i * n + j;
			meshData.Indices32[k + 1] = i * n + j + 1;
			meshData.Indices32[k + 2] = (i + 1)*n + j;

			meshData.Indices32[k + 3] = (i + 1)*n + j;
			meshData.Indices32[k + 4] = i * n + j + 1;
			meshData.Indices32[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	MeshData meshData;

	// Compute the vertices stating at the top pole and moving down the stacks.

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f*XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.Position.x = radius * sinf(phi)*cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi)*sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU.x = -radius * sinf(phi)*sinf(theta);
			v.TangentU.y = 0.0f;
			v.TangentU.z = +radius * sinf(phi)*cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = theta / XM_2PI;
			v.TexC.y = phi / XM_PI;

			meshData.Vertices.push_back(v);
		}
	}

	meshData.Vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		meshData.Indices32.push_back(0);
		meshData.Indices32.push_back(i + 1);
		meshData.Indices32.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);

			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}

	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.

	// South pole vertex was added last.
	uint32_t southPoleIndex = (uint32_t)meshData.Vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices32.push_back(southPoleIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i + 1);
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeoSphere(float radius, uint32_t numSubdivisions)
{
	MeshData meshData;

	// Put a cap on the number of subdivsions.
	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);

	// Approximate a sphere by tessellating an icosahedron

	const float x = 0.525731f;
	const float z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-x, 0.0f, z), XMFLOAT3(x,  0.0f,  z),
		XMFLOAT3(-x, 0.0f, z), XMFLOAT3(x,  0.0f, -z),
		XMFLOAT3(0.0f, z,  x), XMFLOAT3(0.0f,  z, -x),
		XMFLOAT3(0.0f, -z, x), XMFLOAT3(0.0f, -z, -x),
		XMFLOAT3(z,  x, 0.0f), XMFLOAT3(-z,  x, 0.0f),
		XMFLOAT3(z, -x, 0.0f), XMFLOAT3(-z, -x, 0.0f)
	};

	uint32_t k[60] =
	{
		1, 4,  0,	 4, 9, 0,	4, 5, 9,	8, 5, 4,	1, 8, 4,
		1, 10, 8,	10, 3, 8,	8, 3, 5,	3, 2, 5,	3, 7, 2,
		3, 10, 7,	10, 6, 7,	6, 11, 7,	6, 0, 11,	6, 1, 0,
		10, 1, 6,	11, 0, 9,	2, 11, 9,	5, 2, 9,	11, 2, 7
	};

	meshData.Vertices.resize(12);
	meshData.Indices32.assign(&k[0], &k[60]);

	for (uint32_t i = 0; i < 12; ++i)
		meshData.Vertices[i].Position = pos[i];

	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	// Project vertices onto sphere and scale
	for (uint32_t i = 0; i < meshData.Vertices.size(); ++i)
	{
		// Project onto unit sphere
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));

		// Project onto sphere
		XMVECTOR p = radius * n;

		XMStoreFloat3(&meshData.Vertices[i].Position, p);
		XMStoreFloat3(&meshData.Vertices[i].Normal, n);

		// Derive texture coordinates from spherical coordinates.
		float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);

		// Put in[0, 2pi].
		if (theta < 0.0f)
			theta += XM_2PI;

		float phi = acosf(meshData.Vertices[i].Position.y / radius);

		meshData.Vertices[i].TexC.x = theta / XM_2PI;
		meshData.Vertices[i].TexC.y = phi / XM_PI;

		// Partial derivative of P with respect to theta
		meshData.Vertices[i].TangentU.x = -radius * sinf(phi) * sinf(theta);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = radius * sinf(phi) * cosf(theta);

		XMVECTOR tVector = XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(tVector));
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)
{
	MeshData meshData;

	// Build stacks

	float stackHeight = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radiusStep = (topRadius - bottomRadius) / stackCount;

	uint32_t ringCount = stackCount + 1;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (uint32_t i = 0; i < ringCount; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float radius = bottomRadius + i * radiusStep;

		// Vertices of ring
		float dTheta = 2.0f * XM_PI / sliceCount;

		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;

			float cos = cosf(j * dTheta);
			float sin = sinf(j * dTheta);

			vertex.Position = XMFLOAT3(radius*cos, y, radius*sin);

			vertex.TexC.x = (float)j / sliceCount;
			vertex.TexC.y = 1.0f - (float)i / stackCount;

			// Cylinder can be parameteized as follows, where we introduce v parameter
			// that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.

			// Let r0 be the bottom radius and let r1 be the top radius.
			// y(v) = h - hv for v in [0,1]
			// r(v) = r1 + (r0 - r1)v
			//
			// x(t, v) = r(v) * cos(t)
			// y(t, v) = h - hv
			// z(t, v) = r(v) * sin(t)
			//
			// dx/dt = -r(v) * sin(t)
			// dy/dt = 0
			// dz/dt = r(v) * cos(t)
			//
			// dx/dv = (r0-r1) * cos(t)
			// dy/dv = -h
			// dz/dv = (r0-r1) * sin(t)

			// This is unit length
			vertex.TangentU = XMFLOAT3(-sin, 0.0f, cos);

			float dRadius = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dRadius*cos, -height, dRadius * sin);

			XMVECTOR tVector = XMLoadFloat3(&vertex.TangentU);
			XMVECTOR bVector = XMLoadFloat3(&bitangent);
			XMVECTOR nVector = XMVector3Normalize(XMVector3Cross(tVector, bVector));
			XMStoreFloat3(&vertex.Normal, nVector);

			meshData.Vertices.push_back(vertex);

			// Note: observe that the first and last vertex of each ring is duplicated in position,
			// but the texture coordinates are not duplicated. We have to do this so that we can apply
			// textures to cylinders correctly.
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different
	uint32_t ringVertexCount = sliceCount + 1;

	// Compute indices for each stack
	for (uint32_t i = 0; i < stackCount; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			meshData.Indices32.push_back(i * ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.Indices32.push_back(i * ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.Indices32.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);


	return meshData;
}

void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, MeshData& meshData)
{
	uint32_t baseIndex = (uint32_t)meshData.Vertices.size();

	float y = 0.5f * height;
	float dTheta = 2.0f * XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area propertional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex.
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Index of center vertex.
	uint32_t centerIndex = (uint32_t)meshData.Vertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices32.push_back(centerIndex);
		meshData.Indices32.push_back(baseIndex + i + 1);
		meshData.Indices32.push_back(baseIndex + i);
	}
}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, MeshData& meshData)
{
	// Build bottom cap

	uint32_t baseIndex = (uint32_t)meshData.Vertices.size();
	float y = -0.5f * height;

	// vertices of ring
	float dTheta = 2.0f * XM_PI / sliceCount;
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Cache the index of center vertex
	uint32_t centerIndex = (uint32_t)meshData.Vertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices32.push_back(centerIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i + 1);
	}
}

void GeometryGenerator::Subdivide(MeshData& meshData)
{
	// save a copy of the input geometry
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32_t numTris = (uint32_t)inputCopy.Indices32.size() / 3;
	for (uint32_t i = 0; i < numTris; ++i)
	{
		Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
		Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
		Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

		// Generate the midpoints

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v2, v0);

		// Add new geometry

		meshData.Vertices.emplace_back(v0); // 0
		meshData.Vertices.emplace_back(v1); // 1
		meshData.Vertices.emplace_back(v2); // 2
		meshData.Vertices.emplace_back(m0); // 3
		meshData.Vertices.emplace_back(m1); // 4
		meshData.Vertices.emplace_back(m2); // 5

		meshData.Indices32.emplace_back(i * 6 + 0);
		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 5);

		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 4);
		meshData.Indices32.emplace_back(i * 6 + 5);

		meshData.Indices32.emplace_back(i * 6 + 5);
		meshData.Indices32.emplace_back(i * 6 + 4);
		meshData.Indices32.emplace_back(i * 6 + 2);

		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 1);
		meshData.Indices32.emplace_back(i * 6 + 4);
	}

}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1)
{
	XMVECTOR p0 = XMLoadFloat3(&v0.Position);
	XMVECTOR p1 = XMLoadFloat3(&v1.Position);

	XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
	XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

	XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
	XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

	XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
	XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

	// Compute the midpoints of all the attributes.
	// Vectors need to be normalized since linear interpolating can make them not unit length
	XMVECTOR pos = 0.5f * (p0 + p1);
	XMVECTOR normal = XMVector3Normalize(0.4f * (n0 + n1));
	XMVECTOR tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
	XMVECTOR tex = 0.5f * (tex0 + tex1);

	Vertex v;
	XMStoreFloat3(&v.Position, pos);
	XMStoreFloat3(&v.Normal, normal);
	XMStoreFloat3(&v.TangentU, tangent);
	XMStoreFloat2(&v.TexC, tex);

	return v;
}