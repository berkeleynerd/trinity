////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "Curves/TriCurveSet.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshBase.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveUpdateContext.h"

// settings
extern bool g_eveSpaceObjectImpactEffectEnabled;


EveImpactOverlay::EveImpactOverlay( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	m_display( true ),
	m_configuration( IMPACT_INVALID ),
	m_overallShieldImpact( -1.f ),
	m_maxShieldImpacts( 8 ),
	m_shieldEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shieldEllipsoidRadii( 1.f, 1.f, 1.f ),
	m_parentBoundingSphere( 0.f, 0.f, 0.f, -1.f ),
	m_impactDataNextIdx( 1 ),
	m_dataTextureBlockID( -1 ),
	m_dataTextureOffset( -1 ),
	m_armorImpactSizeFactor( 1.f / 77.5f ),
	m_armorImpactSizeMax( 10.f )
{
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
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// do we have something to do at all?
	if( !HasActivity() )
	{
		m_dataTextureBlockID = -1;
		return;
	}

	// the texture buffer needs to be up-to-date to be passed to the shader via texture
	if( m_impactTexelData.size() != std::max( m_shieldImpactData.size(), m_armorImpactData.size() ) )
	{
		return;
	}

	// this comes from the scene via EveUpdateContext
	Tr2DataTextureManagerPtr dataTextureMgr = updateContext.GetDataTextureManager();

	// what's our ofset in pixels for the data texture?
	m_dataTextureOffset = dataTextureMgr->GetTextureOffset( m_dataTextureBlockID );

	// the block header is the first column in the data texture, set it!
	DataRow header;
	header.v[0] = Vector4( float( m_shieldImpactData.size() ), m_overallShieldImpact, 0.f, 0.f );
	header.v[1] = Vector4( 0.f, 0.f, 0.f, 0.f );
	header.v[2] = Vector4( float( m_armorImpactData.size() ), 0.f, 0.f, 0.f );
	header.v[3] = Vector4( 0.f, 0.f, 0.f, 0.f );

	// update block data
	m_dataTextureBlockID = dataTextureMgr->RequestBlockData( &header.v[0], (uint32_t)m_impactTexelData.size(), m_impactTexelData.empty() ? nullptr : &m_impactTexelData[0].v[0] );
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

	// resize the texture data array based on both shield and armor impact
	m_impactTexelData.resize( std::max( m_shieldImpactData.size(), m_armorImpactData.size() ) );

	// no activity?
	if( !HasActivity() )
	{
		return;
	}

	// get parent's bounding ellipsoid shape
	parent->GetShapeEllipsoid( m_shieldEllipsoidCenter, m_shieldEllipsoidRadii );
	// get parent's radius
	parent->GetBoundingSphere( m_parentBoundingSphere );

	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	if( !D3DXMatrixInverse( &parentInverseWorldTransform, nullptr, &parentWorldTransform ) )
	{
		parentInverseWorldTransform = parentWorldTransform;
	}
	
	size_t i = 0;
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); ++sidit )
	{
		ShieldImpactData* shieldData = &sidit->second;
		DataRow* texelData = &m_impactTexelData[i];

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
		texelData->v[0] = Vector4( p, shieldData->timeLeft );
		texelData->v[1] = Vector4( 0.f, 0.f, 0.f, shieldData->lifeTime );
		// also need this intercept position in WS
		D3DXVec3TransformCoord( &shieldData->interceptPosition, &p, &parentWorldTransform );
	
		++i;
	}

	// armor
	i = 0;
	for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
	{
		ArmorImpactData* armorData = &aidit->second;
		DataRow* texelData = &m_impactTexelData[i];

		// size of impact
		float size = armorData->size * std::min( m_armorImpactSizeFactor * m_parentBoundingSphere.w, m_armorImpactSizeMax );
		// get position from damage locator
		Vector3 tgtPosWS( 0.f, 0.f, 0.f );
		parent->GetDamageLocatorPosition( &tgtPosWS, armorData->damageLocatorIndex );
		// convert position and direction into object space
		Vector3 tgtPosOS;
		D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
		texelData->v[2] = Vector4( tgtPosOS, 0.f );
		texelData->v[3] = Vector4( size, 0.f, 0.f, 0.f );

		++i;
	}

	// don't forget the curves
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Update( time, time );
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
	if( m_dataTextureBlockID == -1 )
	{
		return;
	}

	// anything on shields?
	if( HasActivity() )
	{
		const Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
		m_mesh->GetBatches( accumulator, areas, perObjectData );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if this impact overlay is active
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// depends on lists
	return !m_armorImpactData.empty() || !m_shieldImpactData.empty() || ( m_overallShieldImpact > 0.f );
}

// --------------------------------------------------------------------------------
// Description:
//   Just a getter for the texture offset. Nothing special. Move on.
// --------------------------------------------------------------------------------
int32_t EveImpactOverlay::GetDataTextureOffset() const
{
	return m_dataTextureOffset;
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal animation curves
// --------------------------------------------------------------------------------
void EveImpactOverlay::PlayCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal animation curves
// --------------------------------------------------------------------------------
void EveImpactOverlay::StopAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Stop();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets what effects are going to be triggered. Should only change future
//   impacts
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetConfiguration( ImpactConfiguration cfg )
{
	m_configuration = cfg;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new impact effect. Internal states determines
//   what effect to use
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime )
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return -1;
	}

	// what's the situation?
	switch( m_configuration )
	{
	case IMPACT_SHIELD:
		return CreateShieldImpact( damageLocatorIndex, direction, lifeTime );
	case IMPACT_ARMOR:
		return CreateArmorImpact( damageLocatorIndex, 1.f );
	case IMPACT_HULL:
		// todo
		return -1;
	}

	return -1;
}

// --------------------------------------------------------------------------------
// Description:
//   Shield impacts are special, they need constant updating with the direction
//   to the target. Also it returns the actual impact position
// --------------------------------------------------------------------------------
bool EveImpactOverlay::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	// valid?
	if( impactIndex == -1 )
	{
		return false;
	}

	// is it a shield effect?
	auto shieldData = m_shieldImpactData.find( impactIndex );
	if( shieldData != m_shieldImpactData.end() )
	{
		// put new direction in there
		D3DXVec3Normalize( &shieldData->second.direction, &direction );
		// and return the old "intercept" position
		out = shieldData->second.interceptPosition;
		return true;
	}

	// is it an armor effect?
	auto armorData = m_armorImpactData.find( impactIndex );
	if( armorData != m_armorImpactData.end() )
	{
		// nothing to do here
		return true;
	}

	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new shield impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime )
{
	// only need normal
	Vector3 nrmDir;
	D3DXVec3Normalize( &nrmDir, &direction );
	
	// be carefull: try to find an already existing impact, which is close enough!
	int closestImpactIdx = -1;
	float closestImpactAngle = FLT_MIN;
	for( auto it = m_shieldImpactData.begin(); it != m_shieldImpactData.end(); ++it )
	{
		if( damageLocatorIndex == it->second.damageLocatorIndex )
		{
			float a = D3DXVec3Dot( &nrmDir, &it->second.direction );
			if( a > closestImpactAngle )
			{
				closestImpactAngle = a;
				closestImpactIdx = it->first;
			}
		}
	}
	// if we have one that is close enough, use it instead and hand back that index
	if( closestImpactAngle > 0.95f )
	{
		m_shieldImpactData[ closestImpactIdx ].direction = nrmDir;
		m_shieldImpactData[ closestImpactIdx ].timeLeft = 2.f * lifeTime;
		return closestImpactIdx;
	}

	// check size limitation
	if( m_shieldImpactData.size() >= m_maxShieldImpacts )
	{
		return -1;
	}

	// fill our struct, but keep it in world space
	ShieldImpactData sid;
	sid.direction = direction;
	D3DXVec3Normalize( &sid.direction, &sid.direction );
	sid.damageLocatorIndex = damageLocatorIndex;
	sid.interceptPosition = Vector3( 0.f, 0.f, 0.f );
	sid.lifeTime = sid.timeLeft = 2.f * lifeTime;
	m_shieldImpactData[ m_impactDataNextIdx ] = sid;
	return m_impactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new armor impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateArmorImpact( int damageLocatorIndex, float size )
{
	// fill our struct, but keep it in world space
	ArmorImpactData aid;
	aid.damageLocatorIndex = damageLocatorIndex;
	aid.size = size;
	m_armorImpactData[ m_impactDataNextIdx ] = aid;
	return m_impactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor efects
// --------------------------------------------------------------------------------
Tr2EffectPtr EveImpactOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return nullptr;
	}

	// no activity?
	if( !HasActivity() )
	{
		return nullptr;
	}

	if( batchType == TRIBATCHTYPE_DECAL )
	{
		return m_armorDamageShader;
	}
	return nullptr;
}




