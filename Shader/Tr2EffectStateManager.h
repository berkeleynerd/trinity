#pragma once
#ifndef Tr2EffectStateManager_h
#define Tr2EffectStateManager_h

#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2EffectRes );

class TriRenderBatch;
class ITriRenderBatchAccumulator;
class Tr2Effect;
class Tr2LowLevelShader;
class TriVariable;
class Tr2VertexDefinition;
struct Tr2ShaderInputDefinition;

BLUE_DECLARE( Tr2Material );

//
// See http://core/wiki/Tr2EffectStateManager
//
class Tr2EffectStateManager
{
public:
	Tr2EffectStateManager( Tr2RenderContext& renderContext );

	void Initialize();
	void Shutdown();

	// This is called when device is reset
	void ReleaseDeviceResources( TriStorage s );

	void BeginManagedRendering();
	void EndManagedRendering();

	enum
	{
		// 16 textures/samplers per shader
		SAMPLER_MAX_COUNT		= 16,
		VERTEX_STREAM_MAX_COUNT = 16,
		UNKNOWN					= 0XFFFFFFFFu
	};

	enum RenderingMode
	{
		RM_ANY,	// Don't care about the render state
		RM_OPAQUE,
		RM_DECAL,
		RM_DECAL_NO_DEPTH,
		RM_ALPHA,
		RM_ALPHA_ADDITIVE,
		RM_DEPTH_ONLY,
		RM_PICKING,
		RM_FULLSCREEN,
		RM_SPRITE2D,
		RM_CULL,
		RM_LIGHT,
        RM_ERASE,
        RM_PREPASS_COLOR,

		RM_COUNT
	};

	enum 
	{ 
		UNINITIALIZED_DECLARATION = ~0u,
		NULL_DECLARATION = ~0u - 1
	};

	void ApplyStandardStates( RenderingMode rm );

	void ApplyRenderStates( uint32_t ix );
	void ApplyShaderProgram( uint32_t ix );
	Tr2ShaderProgramAL* GetShaderProgram( uint32_t ix );

	void ApplyStreamSource( uint32_t stream, const Tr2BufferAL & buffer, uint32_t offset, uint32_t stride );
	void ApplyIndexBuffer( const Tr2BufferAL & indices );

	// --------------------------------------------------------------------------------------
	// Description:
	//   Set of render states.
	// --------------------------------------------------------------------------------------
	typedef std::map<Tr2RenderContextEnum::RenderState, uint32_t> Tr2RenderStateSetup;


	static uint32_t RegisterShader( 
		Tr2RenderContextEnum::ShaderType type, 
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderBytecodeAL& patchedBytecode,
		const Tr2ShaderSignatureAL& signature );
	static uint32_t RegisterShaderProgram( uint32_t* shaders, size_t count );
	static uint32_t RegisterShaderProgramOverride( uint32_t originalProgram, uint32_t overrideProgram );


	static uint32_t RegisterRenderStateSetup( 
		const Tr2RenderStateSetup& rss );

	void SetWireframeRendering( bool b );
	void SetInvertedCullMode( bool b );
	bool IsCullModeInverted();

	void SetInvertedDepthTest( bool invertedDepthTest );
	bool IsDepthTestInverted() const;

	static uint32_t GetVertexDeclarationHandle( const Tr2VertexDefinition& vertexDefinition );
	void ApplyVertexDeclaration( uint32_t declaration );
	static bool GetVertexDeclarationElements( uint32_t declaration, Tr2VertexDefinition& definition );

private:
	friend class Tr2EffectRes;
	friend class Tr2LowLevelShader;

	void SetRenderStateOverride( Tr2RenderContextEnum::RenderState state, const uint32_t* overrides );
	void DoApplyRenderStates( uint32_t ix );

	Tr2RenderContext&	m_renderContext;

	// used by SetPerObjectDataToDevice
	Tr2ConstantBufferAL		m_perObjectConstantBuffers[ Tr2RenderContextEnum::CBUFFER_COUNT ];
	
	struct CurrentValues
	{
		void Reset();
		
		uint32_t m_shaderProgram;

		uint32_t m_vertexDeclaration;

		struct HalStream
		{
			Tr2BufferAL m_vertexBuffer;
			uint32_t m_offset;
			uint32_t m_stride;
		};
		HalStream m_streams[VERTEX_STREAM_MAX_COUNT];

		Tr2BufferAL m_indexBuffer;
		Tr2EffectStateManager::RenderingMode m_renderingMode;
		uint32_t m_renderStateSetup;
	};

	CurrentValues m_currentValues;

	bool m_isManagedRendering;

	struct RenderStates
	{
		RenderStates()
			:dirty( true )
		{
		}

		std::vector<uint32_t> states;
		bool dirty;
	};

	std::vector<RenderStates> m_renderStates;
	const uint32_t* m_renderStateOverrides[Tr2RenderContextEnum::RS_MAX_STATE];

	Tr2EffectStateManager( const Tr2EffectStateManager & );
	Tr2EffectStateManager& operator=( const Tr2EffectStateManager & );
};

#endif //Tr2EffectStateManager_h
