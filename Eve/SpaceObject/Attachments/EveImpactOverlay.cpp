////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "include/TriMath.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshBase.h"
#include "Eve/EveUpdateContext.h"

EveImpactOverlay::EveImpactOverlay( IRoot* lockobj ):
	m_display( true ),
	m_overallShieldImpact( -1.f ),
	m_maxShieldImpacts( 128 ),
	m_shieldEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shieldEllipsoidRadii( 1.f, 1.f, 1.f ),
	m_shieldImpactDataNextIdx( 1 ),
	m_armorImpactDataNextIdx( 1 )
{
	GlobalStore().RegisterVariable( "ImpactShieldDataMap", &m_shieldDataTexture );

	PrepareResources();
}

EveImpactOverlay::~EveImpactOverlay()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveImpactOverlay::Initialize()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Release all device resources here
// --------------------------------------------------------------------------------
void EveImpactOverlay::ReleaseResources( TriStorage s )
{
	// get rid of data texture
	m_shieldDataTexture.Destroy();
}

// --------------------------------------------------------------------------------
// Description:
//   Debug/dev helper strings to show up in some tools
// --------------------------------------------------------------------------------
#ifdef TRINITYDEV
void EveImpactOverlay::GetDescription( std::string& desc )
{
	desc = std::string( "EveImpactOverlay" );
}
#endif

// --------------------------------------------------------------------------------
// Description:
//   Allocate all device resources here
// --------------------------------------------------------------------------------
bool EveImpactOverlay::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// create the shield impact data texture here, prefill it with zeros
	std::vector<Vector4> prefillData( m_maxShieldImpacts * 2 * IMPACT_DATA_ROW_COUNT, Vector4( 0.f, 0.f, 0.f, 0.f ) );
	Tr2SubresourceData init = { &prefillData, m_maxShieldImpacts * uint32_t(sizeof(Vector4)), 2 * IMPACT_DATA_ROW_COUNT * m_maxShieldImpacts * uint32_t(sizeof(Vector4)) };
	if( FAILED( m_shieldDataTexture.Create2D( m_maxShieldImpacts, 2 * IMPACT_DATA_ROW_COUNT, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_WRITE, &init, renderContext ) ) )
	{
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// update data texture
	void* data = nullptr;
	uint32_t pitch = 0;
	if( SUCCEEDED( m_shieldDataTexture.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) ) )
	{
		uint8_t* mem = (uint8_t*)data;

		// shield info in first lines
		for( uint32_t x = 0; x < m_shieldDataTexture.GetWidth(); ++x )
		{
			Vector4* texelRow0 = (Vector4*)&mem[pitch * 0 + 16 * x];
			Vector4* texelRow1 = (Vector4*)&mem[pitch * 1 + 16 * x];

			// first one is special
			if( x == 0 )
			{
				*texelRow0 = Vector4( float( m_shieldTexelData.size() ), m_overallShieldImpact, 0.f, 0.f );
				memset( texelRow1, 0, sizeof(Vector4) );
			}
			else
			{
				if( x - 1 < m_shieldTexelData.size() )
				{
					*texelRow0 = m_shieldTexelData[ x - 1 ].rows[0];
					*texelRow1 = m_shieldTexelData[ x - 1 ].rows[1];
				}
				else
				{
					memset( texelRow0, 0, sizeof(Vector4) );
					memset( texelRow1, 0, sizeof(Vector4) );
				}
			}
		}

		// armor data on second line
		for( uint32_t x = 0; x < m_shieldDataTexture.GetWidth(); ++x )
		{
			Vector4* texelRow0 = (Vector4*)&mem[pitch * ( IMPACT_DATA_ROW_COUNT + 0 ) + 16 * x];
			Vector4* texelRow1 = (Vector4*)&mem[pitch * ( IMPACT_DATA_ROW_COUNT + 1 ) + 16 * x];

			// first one is special
			if( x == 0 )
			{
				*texelRow0 = Vector4( float( m_armorTexelData.size() ), 0.f, 0.f, 0.f );
				memset( texelRow1, 0, sizeof(Vector4) );
			}
			else
			{
				if( x - 1 < m_armorTexelData.size() )
				{
					*texelRow0 = m_armorTexelData[ x - 1 ].rows[0];
					*texelRow1 = m_armorTexelData[ x - 1 ].rows[1];
				}
				else
				{
					memset( texelRow0, 0, sizeof(Vector4) );
					memset( texelRow1, 0, sizeof(Vector4) );
				}
			}
		}

		m_shieldDataTexture.Unlock( renderContext );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Do all the math-heavy conversion here async
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// first take out the dead ones
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); )
	{
		sidit->second.timeLeft -= updateContext.GetDeltaT();
		if( sidit->second.timeLeft <= 0.f )
		{
			m_shieldImpactData.erase(sidit++);
		}
		else
		{
			++sidit;
		}
	}

	// get parent's bounding ellipsoid shape
	parent->GetShapeEllipsoid( m_shieldEllipsoidCenter, m_shieldEllipsoidRadii );

	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	if( !D3DXMatrixInverse( &parentInverseWorldTransform, nullptr, &parentWorldTransform ) )
	{
		parentInverseWorldTransform = parentWorldTransform;
	}

	m_shieldTexelData.resize( m_shieldImpactData.size() );
	size_t i = 0;
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); ++sidit )
	{
		ShieldImpactData* shieldData = &sidit->second;
		ShieldTexelData* texelData = &m_shieldTexelData[i];

		// get worldpos of damagelocator from parent
		Vector3 tgtPosWS( 0.f, 0.f, 0.f );
		parent->GetDamageLocatorPosition( &tgtPosWS, shieldData->damageLocatorIndex );
		// convert position and direction into object space
		Vector3 tgtPosOS, dirOS;
		D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
		D3DXVec3TransformNormal( &dirOS, &shieldData->direction, &parentInverseWorldTransform );
		// intersections
		Vector3 p( 0.f, 0.f, 0.f );
		IntersectEllipsoidRay( p, m_shieldEllipsoidCenter, m_shieldEllipsoidRadii, tgtPosOS, dirOS );
		// "encode" it in texels
		texelData->rows[0] = Vector4( p, shieldData->timeLeft );
		texelData->rows[1] = Vector4( 0.f, 0.f, 0.f, shieldData->lifeTime );
		// also need this intercept position in WS
		D3DXVec3TransformCoord( &shieldData->interceptPosition, &p, &parentWorldTransform );
	
		++i;
	}

	// armor
	m_armorTexelData.resize( m_armorImpactData.size() );
	i = 0;
	for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
	{
		ArmorImpactData* armorData = &aidit->second;
		ArmorTexelData* texelData = &m_armorTexelData[i];

		// get position from damage locator
		Vector3 tgtPosWS( 0.f, 0.f, 0.f );
		parent->GetDamageLocatorPosition( &tgtPosWS, armorData->damageLocatorIndex );
		// convert position and direction into object space
		Vector3 tgtPosOS;
		D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
		texelData->rows[0] = Vector4( tgtPosOS, 0.f );
		texelData->rows[1] = Vector4( 0.f, 0.f, 0.f, 0.f );

		++i;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( !m_display )
	{
		return;
	}
	if( !m_mesh )
	{
		return;
	}
	if( !m_shieldDataTexture.IsValid() )
	{
		return;
	}

	// anything on shields?
	if( !m_shieldTexelData.empty() || ( m_overallShieldImpact > 0.f ) )
	{
		GlobalStore().RegisterVariable( "ImpactShieldDataMap", &m_shieldDataTexture );
		const Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
		m_mesh->GetBatches( accumulator, areas, perObjectData );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new shield impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime )
{
	// fill our struct, but keep it in world space
	ShieldImpactData sid;
	sid.direction = direction;
	D3DXVec3Normalize( &sid.direction, &sid.direction );
	sid.damageLocatorIndex = damageLocatorIndex;
	sid.interceptPosition = Vector3( 0.f, 0.f, 0.f );
	sid.lifeTime = sid.timeLeft = 2.f * lifeTime;
	m_shieldImpactData[ m_shieldImpactDataNextIdx ] = sid;
	return m_shieldImpactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Shield impacts are special, they need constant updating with the direction
//   to the target
// --------------------------------------------------------------------------------
bool EveImpactOverlay::UpdateShieldImpact( const Vector3& direction, int shieldImpactIndex )
{
	auto finder = m_shieldImpactData.find( shieldImpactIndex );
	if( finder == m_shieldImpactData.end() )
	{
		return false;
	}
	D3DXVec3Normalize( &finder->second.direction, &direction );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the intercept position of a given impact
// --------------------------------------------------------------------------------
bool EveImpactOverlay::GetShieldImpactPosition( Vector3& out, int shieldImpactIndex ) const
{
	auto finder = m_shieldImpactData.find( shieldImpactIndex );
	if( finder == m_shieldImpactData.end() )
	{
		return false;
	}
	out = finder->second.interceptPosition;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new armor impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateArmorImpact( int damageLocatorIndex )
{
	// fill our struct, but keep it in world space
	ArmorImpactData aid;
	aid.damageLocatorIndex = damageLocatorIndex;
	aid.timeLeft = 0.f;
	m_armorImpactData[ m_armorImpactDataNextIdx ] = aid;
	return m_armorImpactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor efects
// --------------------------------------------------------------------------------
Tr2EffectPtr EveImpactOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	if( batchType == TRIBATCHTYPE_DECAL )
	{
		return m_armorDamageShader;
	}
	return nullptr;
}




