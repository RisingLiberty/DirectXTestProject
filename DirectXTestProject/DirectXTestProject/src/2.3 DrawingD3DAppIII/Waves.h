#pragma once

#include <vector>
#include <DirectXMath.h>

class Waves
{
public: 
	Waves(int numRows, int numColumns, float spatialStep, float timeStep, float speed, float damping);
	Waves(const Waves& other) = delete;
	Waves& operator=(const Waves& other) = delete;
	~Waves();

	int GetRowCount() const;
	int GetColumnCount() const;
	int GetVertexCount() const;
	int GetTriangleCount() const;
	float GetWidth() const;
	float GetDepth() const;

	// Returns the solution at the ith grid point.
	const DirectX::XMFLOAT3& GetPosition(int i) const;

	// Returns the solition normal at the ith grid point
	const DirectX::XMFLOAT3& GetNormal(int i) const;

	// Returns the unit tangent vector at the ith grid point in the local x-axis direction
	const DirectX::XMFLOAT3& GetTangentX(int i) const;

	void Update(float dTime);

	void Disturb(int rowIndex, int columnIndex, float magnitude);

private:
	int m_NumRows;
	int m_NumColumns;

	int m_VertexCount = 0;
	int m_TriangleCount = 0;

	// Simulation constants we can precompute
	float m_K1;
	float m_K2;
	float m_K3;

	float m_TimeStep;
	float m_SpatialStep;

	std::vector<DirectX::XMFLOAT3> m_PreviousSolution;
	std::vector<DirectX::XMFLOAT3> m_CurrentSolution;
	std::vector<DirectX::XMFLOAT3> m_Normals;
	std::vector<DirectX::XMFLOAT3> m_TangentX;
};