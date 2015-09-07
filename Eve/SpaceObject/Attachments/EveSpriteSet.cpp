#include "StdAfx.h"
#include "EveSpriteSet.h"
#include "Tr2Effect.h"
#include "TriRenderBatch.h"
#include "Utilities/MatrixUtils.h"
#include "Tr2QuadRenderer.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

const Tr2VertexDefinition& EveSpriteSet::PoolVertex::GetDefinition()
{
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.m_items.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );

		vd.Add( vd.FLOAT32_3, vd.POSITION, 0, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT16_2, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.UBYTE_4_NORM , vd.COLOR, 0, 1, 1 );
	}
	return s_spriteVertexDecl;
}

struct SpriteVertex
{
	Vector3 m_position;
	uint32_t m_color;
	float m_blinkPhase;
	float m_blinkRate;
	float m_minScale;
	float m_maxScale;
	float m_falloff;
	uint8_t m_index;
	uint8_t m_boneIndex;

	uint8_t m_padding[2];
};

EveSpriteSet::EveSpriteSet( IRoot* lockobj ) :
	PARENTLOCK( m_sprites ),
	m_vertexCount( 0 ),
	m_display( true ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_useQuadRenderer( false ),
	m_skinned( false ),
	m_effectHash( 0 ),
	m_buffer( "EveSpriteSet::m_buffer" ),
	m_spriteData( "EveSpriteSet::m_spriteData" )
{
}

EveSpriteSet::~EveSpriteSet()
{
}

bool EveSpriteSet::Initialize()
{
	PrepareResources();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Switches sprite set to use shared quad renderer.
// Arguments:
//   useQuadRenderer - if the quad renderer should be used
//   skinned - if the spotlight set should use skinning (only matters for quad rendering)
// --------------------------------------------------------------------------------
void EveSpriteSet::UseQuadRenderer( bool useQuadRenderer, bool skinned )
{
	if( useQuadRenderer && !m_effect )
	{
		CCP_ASSERT_M( false, "effect must be initialized before using quad renderer" );
		useQuadRenderer = false;
	}
	if( m_useQuadRenderer == useQuadRenderer )
	{
		return;
	}
	m_useQuadRenderer = useQuadRenderer;
	m_skinned = skinned;
	if( m_useQuadRenderer )
	{
		m_effectHash = m_effect->GetHashValue();
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();
	}
	PrepareResources();
}

void EveSpriteSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer.Destroy();
}

bool EveSpriteSet::OnPrepareResources()
{
	if( m_useQuadRenderer )
	{
		int n = (unsigned int)m_sprites.GetSize();
		m_buffer.resize( n );
		for( int i = 0; i < n; ++i )
		{
			auto sprite = m_sprites[i];
			auto& vertex = m_buffer[i];
			vertex.m_position = sprite->m_position;
			uint32_t color = sprite->m_color;
			vertex.m_color = 
				( ( color & 0xff0000 ) >> 16 ) |
				( color & 0xff00ff00 ) |
				( ( color & 0xff ) << 16 );
			vertex.m_blinkPhase = sprite->m_blinkPhase;
			vertex.m_blinkRate = sprite->m_blinkRate;
			vertex.m_minScale = sprite->m_minScale;
			vertex.m_maxScale = sprite->m_maxScale;
			vertex.m_falloff = sprite->m_falloff;
		}

		m_spriteData.resize( n );
		for( int i = 0; i < n; ++i )
		{
			m_spriteData[i].position = m_sprites[i]->m_position;
			m_spriteData[i].boneIndex = m_sprites[i]->m_boneIndex;
		}
		return true;
	}

	if( m_vertexBuffer.IsValid() )
	{
		return true;
	}

	if( m_sprites.empty() )
	{
		return true;
	}

	// register vertex declaration
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.UBYTE_4_NORM , vd.COLOR );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 0 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 1 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 2 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 3 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 4 );
		vd.Add( vd.UBYTE_4  , vd.TEXCOORD, 5 );
	}

	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_spriteVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	m_vertexCount = (unsigned int)m_sprites.GetSize() * 6;

	std::vector<SpriteVertex> pVerts( m_vertexCount );

	int n = (unsigned int)m_sprites.GetSize();
	for( int i = 0; i < n; ++i )
	{
		for( int j = 0; j < 6; ++j )
		{
			SpriteVertex& vertex = pVerts[i*6 + j];
			vertex.m_position = m_sprites[i]->m_position;
			uint32_t color = m_sprites[i]->m_color;
			vertex.m_color = 
				( ( color & 0xff0000 ) >> 16 ) |
				( color & 0xff00ff00 ) |
				( ( color & 0xff ) << 16 );
			vertex.m_blinkPhase = m_sprites[i]->m_blinkPhase;
			vertex.m_blinkRate = m_sprites[i]->m_blinkRate;
			vertex.m_minScale = m_sprites[i]->m_minScale;
			vertex.m_maxScale = m_sprites[i]->m_maxScale;
			vertex.m_falloff = m_sprites[i]->m_falloff;
			vertex.m_boneIndex = m_sprites[i]->m_boneIndex;
		}

		pVerts[i*6 + 0].m_index = 0;
		pVerts[i*6 + 1].m_index = 2;
		pVerts[i*6 + 2].m_index = 1;

		pVerts[i*6 + 3].m_index = 0;
		pVerts[i*6 + 4].m_index = 3;
		pVerts[i*6 + 5].m_index = 2;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( m_vertexBuffer.Create( m_vertexCount * sizeof( SpriteVertex ), USAGE_IMMUTABLE, &pVerts[0], renderContext ), false );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites to render as booster glow with quad renderer if quad rendering 
//   was enabled with UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
//   boosterGain - parent booster intensity
// --------------------------------------------------------------------------------
void EveSpriteSet::AddBoosterGlowToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float boosterGain )
{
	if( !m_useQuadRenderer || !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = Tr2Renderer::GetIdentityTransform();
	auto n = m_spriteData.size();

	XMVector3TransformCoordStream( reinterpret_cast
		<XMFLOAT3*>( &m_buffer[0].m_position ), 
		sizeof( PoolVertex ), 
		reinterpret_cast<XMFLOAT3*>( &m_spriteData[0].position ), 
		sizeof( SpriteData ), 
		uint32_t( n ), 
		world );

	D3DXFLOAT16 zDirX = world.GetZ().x;
	D3DXFLOAT16 zDirY = world.GetZ().y;
	D3DXFLOAT16 zDirZ = world.GetZ().z;
	uint32_t gain = std::min( uint32_t( boosterGain * 255.f ), 255u ) << 24;
	for( size_t i = 0; i < n; ++i )
	{
		auto& vert = m_buffer[i];
		vert.activation = zDirX;
		vert.m_blinkRate = zDirY;
		vert.m_falloff = zDirZ;
		vert.m_color = ( vert.m_color & 0xffffff ) | gain;

	}
	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_sprites.GetSize() );
}

void EveSpriteSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_useQuadRenderer )
	{
		quadRenderer.RegisterEffect( m_effectHash, sizeof( PoolVertex ), 1, PoolVertex::GetDefinition(), m_effect );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites to render with quad renderer if quad rendering was enabled with 
//   UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
//   activation - parent "activation" state
//   bones - array of bone transforms
//   boneCount - number of bones
// --------------------------------------------------------------------------------
void EveSpriteSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float activation, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_useQuadRenderer || !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = Tr2Renderer::GetIdentityTransform();
	auto n = m_spriteData.size();
	if( !m_skinned )
	{
		XMVector3TransformCoordStream( 
			reinterpret_cast<XMFLOAT3*>( &m_buffer[0].m_position ), 
			sizeof( PoolVertex ), 
			reinterpret_cast<XMFLOAT3*>( &m_spriteData[0] ), 
			sizeof( SpriteData ), 
			uint32_t( n ), 
			world );
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
				XMStoreFloat3A( 
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].m_position ), 
					XMVector3TransformCoord( position, world ) );
			}
			else
			{
				XMStoreFloat3A( 
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].m_position ), 
					XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), world ) );
			}
		}
	}
	D3DXFLOAT16 activation16 = activation;
	for( size_t i = 0; i < n; ++i )
	{
		m_buffer[i].activation = activation16;
	}
	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_sprites.GetSize() );
}

void EveSpriteSet::Clear()
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer.Destroy();
	m_sprites.Remove(-1);
}

void EveSpriteSet::Add( const Vector3& pos, float blinkRate, float blinkPhase, float minScale, float maxScale, float falloff, Color color )
{
	EveSpriteSetItemPtr item;
	item.CreateInstance();

	item->m_position = pos;
	item->m_blinkRate = blinkRate;
	item->m_blinkPhase = blinkPhase;
	item->m_minScale = minScale;
	item->m_maxScale = maxScale;
	item->m_falloff = falloff;
	item->m_color = color;
	item->m_boneIndex = 0;

	Add( item );
}

void EveSpriteSet::Add( const Vector3& pos, float scale, Color color )
{
	Add( pos, 0.0f, 0.0f, scale, scale, 0.0f, color );
}

void EveSpriteSet::Add( EveSpriteSetItemPtr newItem )
{
	m_sprites.Append( newItem );
}

void EveSpriteSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( SpriteVertex ) );
	
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawPrimitive( 0, m_vertexCount / 3 );
}

void EveSpriteSet::GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData )
{
	if( !m_vertexBuffer.IsValid() || !m_effect || m_useQuadRenderer )
	{
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetGeometryProvider( this );

		accumulator->Commit( batch );
	}
}

EveSpriteSetItemVector* EveSpriteSet::GetSprites()
{
	return &m_sprites;
}

const char* EveSpriteSet::GetName()
{
	return m_name.c_str();
}

Tr2EffectPtr EveSpriteSet::GetEffect()
{
	return m_effect;
}

void EveSpriteSet::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
}

void EveSpriteSet::SetName( const char* name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual sprites
// --------------------------------------------------------------------------------
void EveSpriteSet::Rebuild()
{
	ReleaseResources( 0 );
	PrepareResources();
}
