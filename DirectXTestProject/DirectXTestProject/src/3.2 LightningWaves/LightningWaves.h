#pragma once

#include <vector>
#include <DirectXMath.h>

class LightningWaves
{
public:
	LightningWaves(int numRows, int numColumns, float spatialStep, float timeStep, float speed, float damping);
	LightningWaves(const LightningWaves& other) = delete;
	LightningWaves& operator=(const LightningWaves& other) = delete;
	~LightningWaves();

	int GetRowCount() const;
	int GetColumnCount() const;
	int GetVertexCount() const;
	int GetTriangleCount() const;
	float GetWidth() const;
	float GetDepth() const;

	// Returns the solution of the grid point at index
	const DirectX::XMFLOAT3& GetPosition(int idx) const;

	// Returns the solution normal of the grid at index
	const DirectX::XMFLOAT3& GetNormal(int idx) const;

	// Returns the unit tangent vector of the grid at index in the local x-axis direction
	const DirectX::XMFLOAT3& GetTangentX(int idx) const;
	
	void Update(float dTime);
	void Disturb(int rowIndex, int columnIndex, float magnitude);

private:
	int m_NumRows;
	int m_NumColumns;

	int m_VertexCount;
	int m_TriangleCount;

	float m_K1;
	float m_K2;
	float m_K3;

	float m_TimeStep;
	float m_SpatialStep;

	std::vector<DirectX::XMFLOAT3> m_PrevSolution;
	std::vector<DirectX::XMFLOAT3> m_CurrentSolution;
	std::vector<DirectX::XMFLOAT3> m_Normals;
	std::vector<DirectX::XMFLOAT3> m_TangentX;
};