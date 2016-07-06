////////////////////////////////////////////////////////////
//
//    Created:   April 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"

#include "EveCustomMask.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveCustomMask::EveCustomMask( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_materialIndex1( 0 ),
	m_isMirrored( false )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Destruction!
// --------------------------------------------------------------------------------
EveCustomMask::~EveCustomMask()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set values
// --------------------------------------------------------------------------------
void EveCustomMask::Setup( const Vector3& position, const Vector3& scaling, const Quaternion& rotation )
{
	m_position = position;
	m_scaling = scaling;
	m_rotation = rotation;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the matrix used to render the debug boudning box if the projection
// --------------------------------------------------------------------------------
void EveCustomMask::GetDebugDrawMatrix( Matrix* matrix, float objectRadius ) const
{
	// scaling includes size!
	Vector3 finalScale( 0.1f * objectRadius, m_scaling.y * objectRadius, m_scaling.z * objectRadius );

	// build matrix
	D3DXMatrixTransformation( matrix, nullptr, nullptr, &finalScale, nullptr, &m_rotation, &m_position );
}

// --------------------------------------------------------------------------------
// Description:
//   Fill in the needed PPT data into the perobject data
// --------------------------------------------------------------------------------
void EveCustomMask::FillPerObjectDataPS( EveSpaceObjectPSData* psData ) const
{
	// projection matrix
	Matrix customMaskTransform, invCustomMaskTransform;
	D3DXMatrixTransformation( &customMaskTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_position );
	D3DXMatrixInverse( &invCustomMaskTransform, nullptr, &customMaskTransform );
	D3DXMatrixTranspose( &psData->customMaskMatrix, &invCustomMaskTransform );

	// material IDs
	psData->customMaskMaterialIDs = Vector4( (float)m_materialIndex1, 0.f, 0.f, 0.f );

	// additional data
	psData->customMaskData = Vector4( 1.f, m_isMirrored ? 1.f : 0.f, 0.f, 0.f );
}


