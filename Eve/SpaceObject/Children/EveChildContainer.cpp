////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"

#include "Utilities/BoundingSphere.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Renderer.h"
#include "Eve/EveUpdateContext.h"
#include "TriObserverLocal.h"
#include "Tr2LightManager.h"
#include "Tr2PointLight.h"


EveChildContainer::EveChildContainer( IRoot* lockobj ) :
	EveChildTransform(),
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	m_display( true ),
	m_hideOnLowQuality( false )
{
}

EveChildContainer::~EveChildContainer()
{
}

void EveChildContainer::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateVisibility( frustum, parentTransform, parentLod );
	}
}

void EveChildContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->GetRenderables( renderables );
	}
}

bool EveChildContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return false;
	}

	bool success = false;
	Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( (*it)->GetBoundingSphere( bSphere ) )
		{
			BoundingSphereSetOrUpdate( bSphere, sphere, success );
			success = true;
		}
	}
	return success;
}

void EveChildContainer::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildContainer::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if (!m_display )
	{
		return;
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildContainer::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateSyncronous( updateContext, nullptr, this );
	}
	for( auto it = m_observers.begin(); it != m_observers.end(); it++ )
	{
		(*it)->Update( m_worldTransform );
	}
}

void EveChildContainer::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	Matrix localToWorldTransform;
	if( spaceObjectParent )
	{
		spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if ( childParent )
	{
		childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else
	{
		return;
	}

	UpdateTransform( localToWorldTransform );

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateAsyncronous( updateContext, nullptr, this );
	}
	
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Update( time, time );
	}
}


void EveChildContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildContainer::ChangeLOD( Tr2Lod lod )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->ChangeLOD( lod );
	}
}

void EveChildContainer::GetLights( Tr2LightManager& lightManager ) const
{
	XMMATRIX worldTransform = m_worldTransform;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetX() ), 
		XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetY() ), XMVector3LengthEst( m_worldTransform.GetZ() ) ) ) ) / 3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}

void EveChildContainer::PlayCurveSet( const std::string& name )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->PlayCurveSet( name );
	}
}

void EveChildContainer::PlayAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Play();
	}
}

void EveChildContainer::StopAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Stop();
	}
}

void EveChildContainer::StopCurveSet( const std::string& name )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Stop();
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->StopCurveSet( name );
	}
}

void EveChildContainer::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Update( time, time );
		}
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateCurveSet( name, time );
	}
}

float EveChildContainer::GetCurveSetDuration( const std::string& name ) const
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return 0.f;
	}

	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			maxDuration = max( maxDuration, (*it)->GetMaxCurveDuration() );
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		maxDuration = max( maxDuration, (*it)->GetCurveSetDuration( name ) );
	}

	return maxDuration;
}

void EveChildContainer::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}