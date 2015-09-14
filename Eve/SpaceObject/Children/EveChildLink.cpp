////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveChildLink.h"

#include "include/TriMath.h"
#include "TriValueBinding.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveChildLink::EveChildLink( IRoot* lockobj ) :
	EveChildMesh( lockobj ),
	PARENTLOCK( m_linkStrengthCurves ),
	PARENTLOCK( m_linkStrengthBindings ),
	m_scale( 1.f ),
	m_direction( 0.f, 0.f, 1.f ),
	m_linkStrength( 0.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
EveChildLink::~EveChildLink()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Syncronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	EveChildMesh::UpdateSyncronous( updateContext, parent );
}

// --------------------------------------------------------------------------------
// Description:
//   Assyncronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// update the special link curves
	for( ITriFunctionVector::const_iterator it = m_linkStrengthCurves.begin(); it != m_linkStrengthCurves.end(); ++it )
	{
		(*it)->UpdateValue( m_linkStrength );
	}

	for( ITr2ValueBindingVector::const_iterator it = m_linkStrengthBindings.begin(); it != m_linkStrengthBindings.end(); ++it )
	{
		(*it)->CopyValue();
	}

	// get parent worldmatrix
	parent->GetLocalToWorldTransform( m_worldTransform );

	// link rotation comes from direction
	Vector3 linkMeshDir( 0.f, 1.f, 0.f );
	Matrix linkRotationMat;
	TriMatrixRotationArc( &linkRotationMat, &linkMeshDir, &m_direction );

	// need inverse rotation-only from worldmatrix
	Matrix invRotationWorldMat;
	TriMatrixRemoveTranslation( &invRotationWorldMat, &m_worldTransform );
	D3DXMatrixInverse( &invRotationWorldMat, nullptr, &invRotationWorldMat );

	// combine inverse to link matrix, so we can do the intersection calculation in axis-aligned space
	D3DXMatrixMultiply( &linkRotationMat, &linkRotationMat, &invRotationWorldMat );

	// update perobject data buffers
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
	parent->GetPerObjectStructs( m_vsData, m_psData );
	m_vsData.worldTransformLast = linkRotationMat;
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
	D3DXMatrixTranspose( &m_vsData.worldTransformLast, &linkRotationMat );

//	EveChildMesh::UpdateAsyncronous( updateContext, parent );
}