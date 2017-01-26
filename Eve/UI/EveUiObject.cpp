////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveUiObject.h"

#include "TriFrustum.h"

// --------------------------------------------------------------------------------
EveUiObject::EveUiObject( IRoot* lockobj ) :
	EveSpaceObject2( lockobj ),
	m_usePerspectiveScale( true )
{
}

// --------------------------------------------------------------------------------
EveUiObject::~EveUiObject()
{
}

// --------------------------------------------------------------------------------
void EveUiObject::UpdateWorldTransform( Be::Time time )
{
	EveSpaceObject2::UpdateWorldTransform( time );

	if( !m_usePerspectiveScale )
	{
		// add a scaling to the worldmatrix based on camera distance to make this guy not perspective
		XMVECTOR d = m_worldPosition - Tr2Renderer::GetViewPosition();
		XMVECTOR dist = XMVector3Length( d );

		XMMATRIX scaleMatrix = XMMatrixScalingFromVector( dist );
		m_worldTransform = XMMatrixMultiply( scaleMatrix, m_worldTransform );
	}
}

// --------------------------------------------------------------------------------
void EveUiObject::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	EveSpaceObject2::UpdateVisibility( frustum, parentTransform );

	// no matter what gets calculated, ui models have NO lod
	m_isVisible = m_isMeshVisible = m_isInFrustum;
	m_lodLevel = TR2_LOD_HIGH;
	m_lodLevelWithChildren = TR2_LOD_HIGH;
	m_impostorMode = false;
}

