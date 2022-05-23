#include "FieldSwapper.h"

#include <assert.h>

FieldSwapper::VectorFieldResources::VectorFieldResources(ID3D11Texture2D* vectorFieldTexture, ID3D11ShaderResourceView* vectorFieldTextureSRV) :
	m_pVectorFieldTexture{ vectorFieldTexture }, m_pVectorFieldTextureSRV{ vectorFieldTextureSRV } {}

FieldSwapper::VectorFieldResources::~VectorFieldResources()
{
	if (m_pVectorFieldTexture)
	{
		m_pVectorFieldTexture->Release();
		m_pVectorFieldTexture = nullptr;
	}

	if (m_pVectorFieldTextureSRV)
	{
		m_pVectorFieldTextureSRV->Release();
		m_pVectorFieldTextureSRV = nullptr;
	}
}

FieldSwapper::FieldSwapper() : m_iCurFieldIndex{ 0 }, m_iCurStepsNum{ 0 }, m_iCurStepsCounter{ 0 } {}

FieldSwapper::~FieldSwapper()
{
	for (size_t i = 0; i < m_aFeildsResources.size(); ++i)
	{
		delete m_aFeildsResources[i];
	}
}

ID3D11ShaderResourceView* FieldSwapper::CurrentVectorFieldSRV() const
{
	return m_aFeildsResources[m_iCurFieldIndex]->m_pVectorFieldTextureSRV;
}

ID3D11ShaderResourceView* FieldSwapper::FieldSRVByIndex(size_t ind) const
{
	if (ind < m_aFeildsResources.size())
	{
		return m_aFeildsResources[ind]->m_pVectorFieldTextureSRV;
	}

	return nullptr;
}

int FieldSwapper::CurrentFieldIndex() const
{
	return (int)m_iCurFieldIndex;
}

int FieldSwapper::CurrentStepsNum() const
{
	return (int)m_iCurStepsNum;
}

///////////////////////////////////
int FieldSwapper::CurrentInterpolateType() const
{
	if (m_aInterplateTypes.size() <= m_iCurFieldIndex)
	{
		return 0;
	}

	return m_aInterplateTypes[m_iCurFieldIndex];
}
///////////////////////////////////

int FieldSwapper::TotalFieldsNum() const
{
	return (int)m_aFeildsResources.size();
}

int FieldSwapper::StepsPerFieldByIndex(size_t ind) const
{
	assert(ind < m_aStepsPerField.size());

	return (int)m_aStepsPerField[ind];
}

void FieldSwapper::AddField(ID3D11Texture2D* vectorFieldTexture, ID3D11ShaderResourceView* vectorFieldTextureSRV)
{
	assert(vectorFieldTexture && vectorFieldTextureSRV);

	m_aFeildsResources.push_back(new VectorFieldResources(vectorFieldTexture, vectorFieldTextureSRV));
}

void FieldSwapper::NextField()
{
	m_iCurFieldIndex = (m_iCurFieldIndex + 1) % m_aFeildsResources.size();
}

void FieldSwapper::SetUpStepPerFiled(std::vector<size_t> const& stepsPerField)
{
	m_aStepsPerField = stepsPerField;
	m_iCurStepsNum = 0;
}

///////////////////////////////////
void FieldSwapper::SetUpInterpolateType(std::vector<int> const& interpolateTypes)
{
	m_aInterplateTypes = interpolateTypes;
}
///////////////////////////////////

void FieldSwapper::IncStep(size_t inc)
{
	for (size_t i = 0; i < inc; ++i)
	{
		++m_iCurStepsCounter;

		if (m_iCurStepsCounter == m_aStepsPerField[m_iCurStepsNum])
		{
			m_iCurStepsNum = (m_iCurStepsNum + 1) % m_aFeildsResources.size();
			m_iCurStepsCounter = 0;

			NextField();
		}
	}
}
