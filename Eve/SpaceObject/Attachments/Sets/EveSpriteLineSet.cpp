////////////////////////////////////////////////////////////
//
//    Created:   January 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"

#include "EveSpriteLineSet.h"

#include "Include/TriMath.h"
#include "Tr2QuadRenderer.h"
#include "Utilities/MatrixUtils.h"
#include "Shader/Tr2Effect.h"
#include "Utilities/BoundingSphere.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSpriteLineSet::EveSpriteLineSet( IRoot* lockobj ) :
	PARENTLOCK( m_spriteLines ),
	m_display( true ),
	m_skinned( false ),
	m_effectHash( 0 ),
	m_buffer( "EveSpriteLineSet::m_buffer" ),
	m_spriteData( "EveSpriteLineSet::m_spriteData" )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Destructor
// --------------------------------------------------------------------------------
EveSpriteLineSet::~EveSpriteLineSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating some stuff
// --------------------------------------------------------------------------------
bool EveSpriteLineSet::Initialize()
{
	PrepareResources();
	CreateBoundingBoxes();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff
// --------------------------------------------------------------------------------
void EveSpriteLineSet::ReleaseResources( TriStorage s )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set it up from the outside
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Setup( Tr2EffectPtr effect, bool isSkinned )
{
	m_effect = effect;
	m_skinned = isSkinned;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a line from the outside
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Add( EveSpriteLineSetItemPtr newItem )
{
	m_spriteLines.Append( newItem );
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual sprites
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Rebuild()
{
	ReleaseResources( 0 );
	PrepareResources();
	CreateBoundingBoxes();
}

// --------------------------------------------------------------------------------------
// Description:
//   Create a bounding sphere around sprite lines. Then create a bounding box from 
//	 the bounding sphere.
// --------------------------------------------------------------------------------------
void EveSpriteLineSet::CreateBoundingBoxes()
{
	// Clear the list before we can rebuild it
	m_boundingBoxes.clear();
	m_aabb = AxisAlignedBoundingBox( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) );
	Vector4 sphere( 0.0f, 0.0f, 0.0f, 0.0f );
	for( auto it = m_spriteLines.begin(); it != m_spriteLines.end(); ++it )
	{
		auto spriteLine = *it;
		size_t numOfSprites = size_t( spriteLine->m_scaling.x );

		if( spriteLine->m_isCircle == false )
		{
			Matrix m = RotationMatrix( spriteLine->m_rotation );
			Vector3 dir = TransformNormal( Vector3( 1.f, 0.f, 0.f ), m );

			auto endPos = spriteLine->m_position + spriteLine->m_spacing * dir * float( numOfSprites );
			BoundingSphereFromPoints( sphere, spriteLine->m_position, endPos );
		}
		else
		{
			auto pos = spriteLine->m_position;
			// In this case the numOfSprites is the diameter of the circle
			sphere = Vector4( pos.x, pos.y, pos.z, float( numOfSprites ) );
		}

		AxisAlignedBoundingBox aabb( sphere );

		// Group together all animated items that are attached to the same bone
		if( spriteLine->m_boneIndex >= 0 )
		{
			m_boundingBoxes.push_back( std::make_pair( spriteLine->m_boneIndex, aabb ) );
		}
		else
		{
			// Group together all static items not attached to any bone
			m_aabb.IncludeBox( aabb );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff
// --------------------------------------------------------------------------------
bool EveSpriteLineSet::OnPrepareResources()
{
	// create a hash value for the effect, global quadrenderer needs it!
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}

	// determine total sprite buffer size
	m_buffer.clear();
	m_spriteData.clear();
	size_t totalBufferidx = 0, totalBufferSize = 0;
	for( auto slit = m_spriteLines.begin(); slit != m_spriteLines.end(); ++slit )
	{
		auto spriteLine = *slit;

		// need matrix for roation
		Matrix m = RotationMatrix( spriteLine->m_rotation );

		// interpret as circle or line?
		if( spriteLine->m_isCircle )
		{
			// how many sprites on this line?
			size_t numOfSprites = size_t( spriteLine->m_spacing );

			// increase buffer
			totalBufferSize += numOfSprites;
			m_buffer.resize( totalBufferSize );
			m_spriteData.resize( totalBufferSize );

			// start populating the sprites from this circle
			float alpha = 0.f;
			EveSpriteSet::PoolVertex* vtx = &m_buffer[totalBufferidx];
			EveSpriteSet::SpriteData* spr = &m_spriteData[totalBufferidx];
			for( size_t i = 0; i < numOfSprites; ++i )
			{
				// position on an ellipsoid in x,z-plane
				Vector3 pos( spriteLine->m_scaling.x * sinf( alpha ), 0.f, spriteLine->m_scaling.y * cosf( alpha ) );
				pos = TransformCoord( pos, m );
				pos += spriteLine->m_position;
				// fill static pool data
				vtx->position = pos;
				vtx->warpColor = vtx->color = ( ( spriteLine->m_color & 0xff0000 ) >> 16 ) | ( spriteLine->m_color & 0xff00ff00 ) | ( ( spriteLine->m_color & 0xff ) << 16 );
				vtx->blinkPhase = Float_16( spriteLine->m_blinkPhase + spriteLine->m_blinkPhaseShift * (float)i );
				vtx->blinkRate = Float_16( spriteLine->m_blinkRate );
				vtx->minScale = Float_16( spriteLine->m_minScale );
				vtx->maxScale = Float_16( spriteLine->m_maxScale );
				vtx->falloff = Float_16( spriteLine->m_falloff );
				vtx->activation = Float_16( 1.f );

				// fill animated pool data
				spr->position = pos;
				spr->boneIndex = spriteLine->m_boneIndex;

				// next
				alpha += TRI_2PI / spriteLine->m_spacing;
				++vtx;
				++spr;
			}

			totalBufferidx = totalBufferSize;
		}
		else
		{
			// how many sprites on this line?
			size_t numOfSprites = size_t( spriteLine->m_scaling.x );

			// increase buffer
			totalBufferSize += numOfSprites;
			m_buffer.resize( totalBufferSize );
			m_spriteData.resize( totalBufferSize );

			// start populating the sprites from this line
			Vector3 pos( spriteLine->m_position );
			Vector3 dir = TransformNormal( Vector3( 1.f, 0.f, 0.f ), m );
			EveSpriteSet::PoolVertex* vtx = &m_buffer[totalBufferidx];
			EveSpriteSet::SpriteData* spr = &m_spriteData[totalBufferidx];
			for( size_t i = 0; i < numOfSprites; ++i )
			{
				// fill static pool data
				vtx->position = pos;
				vtx->warpColor = vtx->color = ( ( spriteLine->m_color & 0xff0000 ) >> 16 ) | ( spriteLine->m_color & 0xff00ff00 ) | ( ( spriteLine->m_color & 0xff ) << 16 );
				vtx->blinkPhase = Float_16( spriteLine->m_blinkPhase + spriteLine->m_blinkPhaseShift * (float)i );
				vtx->blinkRate = Float_16( spriteLine->m_blinkRate );
				vtx->minScale = Float_16( spriteLine->m_minScale );
				vtx->maxScale = Float_16( spriteLine->m_maxScale );
				vtx->falloff = Float_16( spriteLine->m_falloff );
				vtx->activation = Float_16( 1.f );

				// fill animated pool data
				spr->position = pos;
				spr->boneIndex = spriteLine->m_boneIndex;

				// next
				pos += spriteLine->m_spacing * dir;
				++vtx;
				++spr;
			}

			totalBufferidx = totalBufferSize;
		}

	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box around sprite lines, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EveSpriteLineSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box surrounding sprite lines
// --------------------------------------------------------------------------------------
AxisAlignedBoundingBox EveSpriteLineSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	auto aabb = m_aabb;
	for( auto box = m_boundingBoxes.begin(); box != m_boundingBoxes.end(); ++box )
	{
		if( box->first < int( boneCount ) )
		{
			Matrix boneTF = IdentityMatrix();
			TriMatrixCopyFrom3x4( &boneTF, &bones[box->first] );
			auto boxAabb = box->second;
			boxAabb.Transform( boneTF );
			aabb.IncludeBox( boxAabb );
		}
	}
	return aabb;
}

// --------------------------------------------------------------------------------
// Description:
//   Register this set with the global quad render module
// --------------------------------------------------------------------------------
void EveSpriteLineSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_effectHash, TRIBATCHTYPE_ADDITIVE, sizeof( EveSpriteSet::PoolVertex ), 1, EveSpriteSet::PoolVertex::GetDefinition(), m_effect );
}

// --------------------------------------------------------------------------------
void EveSpriteLineSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = IdentityMatrix();
	auto n = m_spriteData.size();
	if( !m_skinned )
	{
		XMVector3TransformCoordStream(
			reinterpret_cast<XMFLOAT3*>( &m_buffer[0].position ),
			sizeof( EveSpriteSet::PoolVertex ),
			reinterpret_cast<XMFLOAT3*>( &m_spriteData[0] ),
			sizeof( EveSpriteSet::SpriteData ),
			uint32_t( n ),
			parentTransform );
	}
	else
	{
		for( size_t i = 0; i < n; ++i )
		{
			auto boneIndex = m_spriteData[i].boneIndex;
			if( boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &m, &bones[boneIndex] );
				XMVECTOR position = XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), m );
				XMStoreFloat3(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( position, parentTransform ) );
			}
			else
			{
				XMStoreFloat3(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), parentTransform ) );
			}
		}
	}

	Float_16 activation16( activation );
	for( size_t i = 0; i < n; ++i )
	{
		m_buffer[i].activation = activation16;
	}

	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_buffer.size() );
}

void EveSpriteLineSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}
