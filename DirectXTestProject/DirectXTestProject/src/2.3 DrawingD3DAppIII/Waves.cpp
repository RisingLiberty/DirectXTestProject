#include "Waves.h"

#include <DirectXMath.h>
#include <ppl.h>
#include <cassert>

using namespace DirectX;

Waves::Waves(int numRows, int numColumns, float spatialStep, float timeStep, float speed, float damping) :
	m_NumRows(numRows),
	m_NumColumns(numColumns),
	m_VertexCount(numRows*numColumns),
	m_TriangleCount((numRows - 1)*(numColumns - 1) * 2),
	m_TimeStep(timeStep),
	m_SpatialStep(spatialStep)
{
	float d = damping * timeStep * 2.0f;
	float e = (speed*speed)*(timeStep*timeStep) / (spatialStep*spatialStep);
	m_K1 = (damping*timeStep - 2.0f) / d;
	m_K2 = (4.0f - 8.0f * e) / d;
	m_K3 = (2.0f * e) / d;

	m_PreviousSolution.resize(numRows * numColumns);
	m_CurrentSolution.resize(numRows * numColumns);
	m_Normals.resize(numRows * numColumns);
	m_TangentX.resize(numRows * numColumns);

	// Generate grid vertices in system memory

	float halfWidth = (numRows - 1) * spatialStep * 0.5f;
	float halfDepth = (numColumns - 1) * spatialStep * 0.5f;
	for (int column = 0; column < numColumns; ++column)
	{
		float z = halfDepth - column * spatialStep;
		for (int row = 0; row < numRows; ++row)
		{
			float x = -halfWidth + row * spatialStep;

			m_PreviousSolution[column*numColumns + row] = XMFLOAT3(x, 0.0f, z);
			m_CurrentSolution[column*numColumns + row] = XMFLOAT3(x, 0.0f, z);
			m_Normals[column*numColumns + row] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_TangentX[column * numColumns + row] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}
	
Waves::~Waves()
{

}

int Waves::GetRowCount() const
{
	return m_NumRows;
}

int Waves::GetColumnCount() const
{
	return m_NumColumns;
}

int Waves::GetVertexCount() const
{
	return m_VertexCount;
}

int Waves::GetTriangleCount() const
{
	return m_TriangleCount;
}

float Waves::GetWidth() const
{
	return m_NumColumns * m_SpatialStep;
}

float Waves::GetDepth() const
{
	return m_NumRows * m_SpatialStep;
}


void Waves::Update(float dTime)
{
	static float t = 0;

	// Accumulate time
	t += dTime;

	// Only update the simulation after the specified time step
	if (t >= m_TimeStep)
	{
		
		// Only update interior points. we use zero boundary conditions
		concurrency::parallel_for(1, m_NumRows - 1,
			[this](int row)
		{
			for (int column = 1; column < m_NumColumns - 1; ++column)
			{
				// After this update we will be discarding the old previous bufer,
				// so overwrite that buffer with the new update.
				// Note: how we can do this inplace (read/write to same element)
				// because we won't need prev_ij again and the assignment happens last.

				// Note: column indexes x and row indexes z: h(x_column, z_row, t_k)
				// moreover, our +z axis goes "down".
				// This is just to keep consistent with out row indices going down.
				m_PreviousSolution[row * m_NumColumns + column].y =
					m_K1 * m_PreviousSolution[row * m_NumColumns + column].y +
					m_K2 * m_CurrentSolution[row * m_NumColumns + column].y +
					m_K3 * (m_CurrentSolution[(row + 1) * m_NumColumns + column].y +
							m_CurrentSolution[(row - 1) * m_NumColumns + column].y +
							m_CurrentSolution[row * m_NumColumns + column + 1].y +
							m_CurrentSolution[row * m_NumColumns + column - 1].y);
			}
		});

		// We just overwrote the previous buffer with the new data,
		// so this data needs to become the current solution and the old current
		// current solution becomes the new previous solution
		std::swap(m_PreviousSolution, m_CurrentSolution);

		t = 0.0f; // Reset time

		// Compute normals using finite difference scheme
		concurrency::parallel_for(1, m_NumRows - 1,
			[this](int row)
		{
			for (int column = 1; column < m_NumColumns - 1; ++column)
			{
				float left = m_CurrentSolution[row * m_NumColumns + column - 1].y;
				float right = m_CurrentSolution[row * m_NumColumns + column - 1].y;
				float top = m_CurrentSolution[(row - 1) * m_NumColumns + column].y;
				float bottom = m_CurrentSolution[(row + 1) * m_NumColumns + column].y;

				m_Normals[row * m_NumColumns + column].x = -right + left;
				m_Normals[row * m_NumColumns + column].y = 2.0f * m_SpatialStep;
				m_Normals[row * m_NumColumns + column].z = bottom - top;

				XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&m_Normals[row * m_NumColumns + column]));
				XMStoreFloat3(&m_Normals[row * m_NumColumns] + column, n);

				m_TangentX[row * m_NumColumns + column] = XMFLOAT3(2.0f * m_SpatialStep, right - left, 0.0f);
				XMVECTOR tangent = XMVector3Normalize(XMLoadFloat3(&m_TangentX[row * m_NumColumns + column]));
				XMStoreFloat3(&m_TangentX[row * m_NumColumns + column], tangent);
			}
		});
	}
}

void Waves::Disturb(int rowIndex, int columnIndex, float magnitude)
{
	// Don't disturb boundaries
	assert(rowIndex > 1 && rowIndex < m_NumRows - 2);
	assert(columnIndex > 1 && columnIndex < m_NumColumns - 2);

	float halfMag = 0.5f * magnitude;

	// Disturb the vertex height of gird[rowIndex, columnIndex] and its nieghbors
	int i = rowIndex;
	int j = columnIndex;

	m_CurrentSolution[i * m_NumColumns + j].y		+= magnitude;
	m_CurrentSolution[i * m_NumColumns + j + 1].y	+= halfMag;
	m_CurrentSolution[i * m_NumColumns + j - 1].y	+= halfMag;
	m_CurrentSolution[(i + 1) * m_NumColumns + j].y += halfMag;
	m_CurrentSolution[(i - 1) * m_NumColumns + j].y += halfMag;
	
}

const XMFLOAT3& Waves::GetPosition(int i) const
{
	return m_CurrentSolution[i];
}

const XMFLOAT3& Waves::GetNormal(int i) const
{
	return m_Normals[i];
}

const XMFLOAT3& Waves::GetTangentX(int i) const
{
	return m_TangentX[i];
}