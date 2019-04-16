#include "LightningWaves.h"

#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace DirectX;

LightningWaves::LightningWaves(int numRows, int numColumns, float spatialStep, float timeStep, float speed, float damping):
	m_NumRows(numRows),
	m_NumColumns(numColumns),
	m_VertexCount(numRows*numColumns),
	m_TriangleCount((numRows - 1)*(numColumns - 1) * 2),
	m_TimeStep(timeStep),
	m_SpatialStep(spatialStep)
{
	float d = damping * timeStep + 2.0f;
	float e = (speed * speed)*(timeStep*timeStep) / (spatialStep*spatialStep);

	m_K1 = (damping*timeStep - 2.0f) / d;
	m_K2 = (4.0f - 8.0f * e) / d;
	m_K3 = (2.0f * e) / d;

	m_PrevSolution.resize(numRows * numColumns);
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

			int idx = column * numColumns + row;
			m_PrevSolution[idx] = XMFLOAT3(x, 0.0f, z);
			m_CurrentSolution[idx] = XMFLOAT3(x, 0.0f, z);
			m_Normals[idx] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_TangentX[idx] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}

LightningWaves::~LightningWaves()
{

}

int LightningWaves::GetRowCount() const
{
	return m_NumRows;
}

int LightningWaves::GetColumnCount() const
{
	return m_NumColumns;
}

int LightningWaves::GetVertexCount() const
{
	return m_VertexCount;
}

int LightningWaves::GetTriangleCount() const
{
	return m_TriangleCount;
}

float LightningWaves::GetWidth() const
{
	return m_NumColumns * m_SpatialStep;
}

float LightningWaves::GetDepth() const
{
	return m_NumRows * m_SpatialStep;
}

const DirectX::XMFLOAT3 & LightningWaves::GetPosition(int idx) const
{
	return m_CurrentSolution[idx];
}

const DirectX::XMFLOAT3 & LightningWaves::GetNormal(int idx) const
{
	return m_Normals[idx];
}

const DirectX::XMFLOAT3 & LightningWaves::GetTangentX(int idx) const
{
	return m_TangentX[idx];
}

void LightningWaves::Update(float dTime)
{
	static float t = 0;

	// Accumulate time
	t += dTime;

	// Only update the simulation at the specified time step
	if (t >= m_TimeStep)
	{
		concurrency::parallel_for(1, m_NumRows - 1, 
			[this](int row)
		{
			for (int column = 1; column < m_NumColumns; ++column)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.
				
				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.
				m_PrevSolution[row*m_NumColumns + column].y =
					m_K1 * m_PrevSolution[row*m_NumColumns + column].y +
					m_K2 * m_CurrentSolution[row*m_NumColumns + column].y +
					m_K3 * (m_CurrentSolution[(row + 1)*m_NumColumns + column].y +
						m_CurrentSolution[(row - 1)*m_NumColumns + column].y +
						m_CurrentSolution[row*m_NumColumns + column + 1].y +
						m_CurrentSolution[row*m_NumColumns + column - 1].y);
			}
		});

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(m_PrevSolution, m_CurrentSolution);

		t = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		concurrency::parallel_for(1, m_NumRows - 1,
			[this](int row)
		{
			for (int column = 1; column < m_NumColumns; ++column)
			{
				float l = m_CurrentSolution[row*m_NumColumns + column - 1].y;
				float r = m_CurrentSolution[row*m_NumColumns + column + 1].y;
				float t = m_CurrentSolution[(row - 1)*m_NumColumns + column].y;
				float b = m_CurrentSolution[(row + 1)*m_NumColumns + column].y;
				m_Normals[row*m_NumColumns + column].x = -r + l;
				m_Normals[row*m_NumColumns + column].y = 2.0f*m_SpatialStep;
				m_Normals[row*m_NumColumns + column].z = b - t;

				XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&m_Normals[row*m_NumColumns + column]));
				XMStoreFloat3(&m_Normals[row*m_NumColumns + column], n);

				m_TangentX[row*m_NumColumns + column] = XMFLOAT3(2.0f*m_SpatialStep, r - l, 0.0f);
				XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&m_TangentX[row*m_NumColumns + column]));
				XMStoreFloat3(&m_TangentX[row*m_NumColumns + column], T);
			}
		});
	}
}

void LightningWaves::Disturb(int rowIndex, int columnIndex, float magnitude)
{
	// Don't disturb boundaries
	assert(rowIndex > 1 && rowIndex < m_NumRows - 2);
	assert(columnIndex > 1 && columnIndex < m_NumColumns - 2);

	float halfMag = 0.5f * magnitude;

	// Disturb the ijth vertex height and its neighbors
	m_CurrentSolution[rowIndex*m_NumRows + columnIndex].y += magnitude;
	m_CurrentSolution[rowIndex*m_NumRows + columnIndex + 1].y += halfMag;
	m_CurrentSolution[rowIndex*m_NumRows + columnIndex - 1].y += halfMag;
	m_CurrentSolution[(rowIndex + 1)*m_NumRows + columnIndex].y += halfMag;
	m_CurrentSolution[(rowIndex - 1)*m_NumRows + columnIndex].y += halfMag;

}
