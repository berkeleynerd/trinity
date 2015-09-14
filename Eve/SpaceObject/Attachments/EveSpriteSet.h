#pragma once
#ifndef EveSpriteSet_H
#define EveSpriteSet_H


#include "Tr2DeviceResource.h"
#include "ITr2GeometryProvider.h"
#include "EveSpriteSetItem.h"
#include "ITr2Renderable.h"


BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );
BLUE_DECLARE( Tr2Effect );

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class Tr2QuadRenderer;

BLUE_CLASS( EveSpriteSet ):
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpriteSet( IRoot* lockobj = NULL );
	~EveSpriteSet();

	// Note: Call Clear, Add (as many times as needed), then PrepareResources
	void Clear();
	void Add( const Vector3& pos, float blinkRate, float blinkPhase, float minScale, float maxScale, float falloff, Color color );
	void Add( const Vector3& pos, float scale, Color color );
	void Add( EveSpriteSetItemPtr newItem );
	
	// Rebuild resources
	void Rebuild();

	void GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData );

	void UseQuadRenderer( bool useQuadRenderer, bool skinned );
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );

	void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float activation, const granny_matrix_3x4* bones, size_t boneCount );
	void AddBoosterGlowToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float boosterGain );

	EveSpriteSetItemVector* GetSprites();
	const char* GetName();
	void SetName( const char* name );

	Tr2EffectPtr GetEffect();
	void SetEffect( Tr2EffectPtr effect );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
private:
	// Persisted members
	bool m_display;
	bool m_useQuadRenderer;
	bool m_skinned;
	std::string m_name;
	unsigned m_effectHash;

	PEveSpriteSetItemVector m_sprites;
	Tr2EffectPtr m_effect;

	// Member variables for runtime state
	unsigned int m_vertexDeclHandle;
	unsigned int m_vertexCount;
	Tr2VertexBufferAL m_vertexBuffer;

	struct PoolVertex
	{
		Vector3 m_position;
		D3DXFLOAT16 activation;
		D3DXFLOAT16 m_blinkPhase;
		D3DXFLOAT16 m_blinkRate;
		D3DXFLOAT16 m_minScale;
		D3DXFLOAT16 m_maxScale;
		D3DXFLOAT16 m_falloff;
		uint32_t m_color;

		static const Tr2VertexDefinition& GetDefinition();
	};
	TrackableStdVector<PoolVertex> m_buffer;

	struct SpriteData
	{
		Vector3 position;
		uint32_t boneIndex;
	};
	TrackableStdVector<SpriteData> m_spriteData;
};

TYPEDEF_BLUECLASS( EveSpriteSet );

#endif // EveSpriteSet_H