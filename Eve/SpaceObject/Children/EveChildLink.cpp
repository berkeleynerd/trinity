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

	// need some matrices to position this guy
	Matrix localRotationMat, localToWorldMat;

	// we need the size of the parent ship to scale the link object
	Vector3 ellipsoidData( 1.f, 1.f, 1.f ), parentBBoxMin( -1.f, -1.f, -1.f ), parentBBoxMax( 1.f, 1.f, 1.f );
	if( parent->GetLocalBoundingBox( parentBBoxMin, parentBBoxMax ) )
	{
		ellipsoidData = m_scale * ( parentBBoxMax - parentBBoxMin );
	}

	// need world matrix from parent, but only translation
	parent->GetLocalToWorldTransform( localToWorldMat );
	D3DXMatrixTranslation( &localToWorldMat, localToWorldMat.GetTranslation().x, localToWorldMat.GetTranslation().y, localToWorldMat.GetTranslation().z );

	// local rotation comes from direction
	Vector3 linkMeshDir( 0.f, 1.f, 0.f );
	TriMatrixRotationArc( &localRotationMat, &linkMeshDir, &m_direction );

	// update the worldmatrix
	D3DXMatrixMultiply( &m_worldTransform, &localRotationMat, &localToWorldMat );

	// update perobject data buffers
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
	parent->GetPerObjectStructs( m_vsData, m_psData );
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
//	m_vsData.ellpsoidData = Vector4( ellipsoidData, 0.f );

//	EveChildMesh::UpdateAsyncronous( updateContext, parent );
}