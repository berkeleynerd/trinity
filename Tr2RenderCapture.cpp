#include "StdAfx.h"
#include "Tr2RenderCapture.h"

#include <fstream>
#include <iomanip>
#include <string>

bool g_trinityCaptureRecord = false;
bool g_trinityCapturePlay = false;
int g_trinityCaptureDebugViewRt = -1;

#if TRINITY_AL_CAPTURE_ENABLED

Tr2RenderCapture	s_renderCapture;

using namespace Tr2RenderContextEnum;

namespace
{

// TODO: implement this if we ever use capture
bool SaveTextureToDds( const Tr2TextureAL& texture, const wchar_t* filename )
{
	return false;
}

}


struct Tr2RenderCapture::Impl
{
	enum
	{
		MAGIC_DEFAULT_BACKBUFFER	= 0x8000000u
	};

	struct ParamSetStreamSource
	{
		uint32_t	stream;
		uint32_t	buffer;
		uint32_t	offset;
		uint32_t	stride;
	};

	struct ParamSetConstants
	{
		uint32_t	buffer;
		ShaderType	constantType;
		uint32_t	registerIndex;
		uint32_t	unusedArgument;
	};

	struct ParamSetShaderBuffer
	{
		ShaderType	inputType;
		uint32_t	slot;
		uint32_t	buffer;
	};

	struct ParamSetTexture
	{
		ShaderType	inputType;
		uint32_t	slot;
		uint32_t	texture;
		Tr2RenderContextEnum::ColorSpace colorSpace;
		uint32_t	wrappedObjectAL;
		uint32_t	wrappedObjectType;
	};

	struct ParamDrawPrimitive
	{
		uint32_t	numVertices;
		uint32_t	startIndex;
		uint32_t	primitiveCount;
		uint32_t	minimumIndex;
		uint32_t	numInstances;
	};

	struct ParamDrawPrimitiveUP
	{
		uint32_t	numVertices;
		uint32_t	primitiveCount;
		uint32_t	indexBuffer;
		uint32_t	vertexBuffer;
		uint32_t	vertexZeroStride;
		IndexBufferBitcount		bitCount;
	};

	struct ParamSetClipPlane
	{
		uint32_t	planeIndex;
		float		planeEq[4];
	};

	struct ParamSetScissorRect
	{
		uint32_t	left;
		uint32_t	top;
		uint32_t	right;
		uint32_t	bottom;
	};

	struct ParamSetSamplerState
	{
		uint32_t	buffer;
		ShaderType	inputType;
		uint32_t	registerNumber;
	};

	struct ParamClear
	{
		uint32_t	clearFlags;
		uint32_t	color;
		float		depth;
		uint32_t	stencil;
	};

	struct ParamSetViewport
	{
		float		vp[6];
	};

	struct ParamMisc
	{
		uint32_t					u32, u32_bis;
		uint32_t					objectAL;
	};

	union Params
	{
		ParamSetStreamSource		setStreamSource;
		ParamSetConstants			setConstants;
		ParamSetShaderBuffer		setShaderBuffer;
		ParamSetTexture				setTexture;
		ParamDrawPrimitive			drawPrimitive;
		ParamDrawPrimitiveUP		drawPrimitiveUP;
		ParamSetClipPlane			setClipPlane;
		ParamSetScissorRect			setScissorRect;
		ParamSetSamplerState		setSamplerState;
		ParamClear					clear;
		ParamSetViewport			setViewport;
		ParamMisc					misc;
	};

	enum Call
	{	
		BEGIN_SCENE,
		END_SCENE,
	
		SET_STREAM_SOURCE,
		SET_INDICES,
		SET_TOPOLOGY,
		SET_VERTEX_LAYOUT,
		SET_SHADER,

		SET_SHADER_BUFFER,
		SET_TEXTURE,

		DRAW_INDEXED_PRIMITIVE,
		DRAW_PRIMITIVE,
		DRAW_INDEXED_INSTANCED,

		DRAW_INDEXED_PRIMITIVE_UP,
		DRAW_PRIMITIVE_UP,

		SET_RENDER_STATE,
		SET_RENDER_STATES,

		SET_CLIP_PLANE,
		SET_SCISSOR_RECT,
		SET_SAMPLER_STATE,

		SET_CONSTANTS,

		SET_DEPTH_STENCIL,
		SET_READ_ONLY_DEPTH,
		SET_RENDER_TARGET,

		CLEAR,

		SET_NUMBER_OF_LIGHTS,
		SET_VIEWPORT,

		PUSH_RENDER_TARGET,
		POP_RENDER_TARGET,

		PUSH_DEPTH_STENCIL,
		POP_DEPTH_STENCIL,

		RESOLVE,

		INVALID_CALL
	};

	struct CapturedCall
	{
		Call	m_call;
		Params	m_params;
	};

	std::vector<CapturedCall>	m_calls;

	template<typename TypeAL, bool delayCopy>
	struct Tr2CapturedResources
	{
		struct Record
		{
			Record(		const TypeAL*	source, 
						//uint32_t		writeLockCount,
						TypeAL*			clone )
				: m_source( source )
				//, m_writeLockCount( writeLockCount )
				, m_clone( clone )
			{}

			//ConstTypeAL*				m_source;
			// deliberately lose the type to make it clear we only keep this as a unique ID,
			// not as an object to call methods on -- the actual object may be dead/non-existing.
			const void*					m_source;
			//uint32_t					m_writeLockCount;
			std::shared_ptr<TypeAL>		m_clone;
		};

		std::vector<Record>	m_vec;

		std::vector<size_t>	m_delayedClone;

		uint32_t	FindOrAdd( const TypeAL& source )
		{
			for( size_t i = 0; i != m_vec.size(); ++i )
			{
				if( m_vec[i].m_source == &source && 
					m_vec[i].m_clone.get() &&
					( !source.IsValid() || 
						( m_vec[i].m_clone->IsValid() &&
						 m_vec[i].m_clone->m_writeLockCount == source.m_writeLockCount ) ) )
				{
					return uint32_t( i );
				}
			}

			m_vec.emplace_back( Record( &source, /*source.m_writeLockCount, */new TypeAL ) );
			if( source.IsValid() )
			{
				if( !delayCopy )
				{
					const_cast<TypeAL& >( source ).CloneTo( *m_vec.back().m_clone );
				}
				else
				{
					m_delayedClone.push_back( m_vec.size() - 1 );
				}
			}

			return uint32_t( m_vec.size()-1 );
		}

		void ExecuteDelayedClones()
		{
			for( size_t i = 0; i != m_delayedClone.size(); ++i )
			{
				auto& it = m_vec[m_delayedClone[i]];
				CR_RETURN( ((TypeAL*)it.m_source)->CloneTo( *it.m_clone ) );
			}
			m_delayedClone.clear();
		}

		void Clear()
		{
			m_delayedClone.clear();
			m_vec.clear();
		}

		uint32_t	CountUnique()
		{
			std::set<const void*>	sources;
			uint32_t count = 0;
			for( size_t i = 0; i != m_vec.size(); ++i )
			{
				if( sources.insert( m_vec[i].m_source ).second )
				{
					++count;
				}
			}
			return count;
		}
	};

	Tr2CapturedResources<Tr2ConstantBufferAL,	true>	m_cb;
	Tr2CapturedResources<Tr2VertexBufferAL,		true>	m_vb;
	Tr2CapturedResources<Tr2IndexBufferAL,		true>	m_ib;
	Tr2CapturedResources<Tr2VertexLayoutAL,		false>	m_vl;
	Tr2CapturedResources<Tr2ShaderAL,			false>	m_sh;
	Tr2CapturedResources<Tr2DepthStencilAL,		false>	m_ds;
	Tr2CapturedResources<Tr2RenderTargetAL,		false>	m_rt;
	Tr2CapturedResources<Tr2TextureAL,			false>	m_tx;
	Tr2CapturedResources<Tr2SamplerStateAL,		false>	m_ss;

	struct RawBufferCreateData
	{
		const void*			m_source;
		CcpMallocBuffer		m_copy;

		RawBufferCreateData() {}

		RawBufferCreateData( RawBufferCreateData&& other )
			: m_source( other.m_source )
			, m_copy( std::move( other.m_copy ) )
		{}

		RawBufferCreateData& operator=( RawBufferCreateData&& other )
		{
			m_source = other.m_source;
			m_copy = std::move( other.m_copy );
			return *this;
		}
	};
	std::vector<RawBufferCreateData>	m_rb;

	struct UavBufferCreateData
	{
		const Tr2GpuBufferAL*	m_uav;

		//TODO everything needed to recreate it goes here
		
	};
	std::vector<UavBufferCreateData>	m_uav;


	void	Clear()
	{
		m_calls.clear();
		m_vb.Clear();
		m_ib.Clear();
		m_cb.Clear();
		m_vl.Clear();
		m_sh.Clear();

		m_ss.Clear();
		m_uav.clear();
		
		m_tx.Clear();
		m_rb.clear();
		m_ds.Clear();
		m_rt.Clear();
	}

	uint32_t	FindOrAdd( const Tr2GpuBufferAL& uav )
	{
		for( size_t i = 0; i != m_uav.size(); ++i )
		{
			if( m_uav[i].m_uav == &uav )
			{
				return uint32_t( i );
			}
		}

		//TODO store creation parameters
		UavBufferCreateData cd;
		cd.m_uav = &uav;
		m_uav.push_back( cd );

		return uint32_t( m_uav.size()-1 );
	}

	uint32_t	FindOrAdd( const void* buf, size_t size )
	{
		for( size_t i = 0; i != m_rb.size(); ++i )
		{
			if( m_rb[i].m_source == buf && 
				m_rb[i].m_copy.size() == size )
			{
				return uint32_t( i );
			}
		}

		RawBufferCreateData cd;
		cd.m_source = buf;
		cd.m_copy.resize( "RawBufferCreateData", size );
		if( cd.m_copy.get() )
		{
			memcpy( cd.m_copy.get(), buf, size );
		}
		m_rb.emplace_back( std::move( cd ) );

		return uint32_t( m_rb.size()-1 );
	}

	ALResult SetStreamSource(		uint32_t stream, 
									const Tr2VertexBufferAL & buffer, 
									uint32_t offset, 
									uint32_t stride ) throw()
	{
		CapturedCall call;
		call.m_call = SET_STREAM_SOURCE;
		auto& param = call.m_params.setStreamSource;

		param.stream = stream;
		param.buffer = m_vb.FindOrAdd( buffer );
		param.offset = offset;
		param.stride = stride;

		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetIndices( const Tr2IndexBufferAL & buffer ) throw()
	{
		CapturedCall call;
		call.m_call = SET_INDICES;
		call.m_params.misc.objectAL	= m_ib.FindOrAdd( buffer );
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetTopology( Topology topology ) throw()
	{
		CapturedCall call;
		call.m_call = SET_TOPOLOGY;
		call.m_params.misc.u32 = topology;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetVertexLayout( Tr2VertexLayoutAL& layout ) throw()
	{
		CapturedCall call;
		call.m_call = SET_VERTEX_LAYOUT;
		call.m_params.misc.objectAL	= m_vl.FindOrAdd( layout );
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetShader( const Tr2ShaderAL& shader ) throw()
	{
		CapturedCall call;
		call.m_call = SET_SHADER;
		call.m_params.misc.objectAL	= m_sh.FindOrAdd( shader );
		call.m_params.misc.u32		= shader.GetType();
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetShaderBuffer(		Tr2RenderContextEnum::ShaderType inputType, 
									uint32_t slot, 
									const Tr2GpuBufferAL& buffer ) throw()
	{
		CapturedCall call;
		call.m_call = SET_SHADER_BUFFER;
		auto& param = call.m_params.setShaderBuffer;

		param.inputType = inputType;
		param.slot		= slot;
		param.buffer	= FindOrAdd( buffer );

		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetTexture(			Tr2RenderContextEnum::ShaderType inputType, 
									uint32_t slot, 
									const Tr2TextureAL& texture, 
									Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR ) throw()
	{
		CapturedCall call;
		call.m_call = SET_TEXTURE;
		auto& param = call.m_params.setTexture;

		param.inputType	= inputType;
		param.slot		= slot;
		param.colorSpace = colorSpace;

		if( !texture.IsAlias() )
		{
			param.texture	= m_tx.FindOrAdd( texture );
			param.wrappedObjectAL = 0;
			param.wrappedObjectType = 0;
		}
		else
		{
			param.texture	= (uint32_t)&texture;	//TODO won't work in 64 bit
			param.wrappedObjectAL = 0xffFFffFFu;
			param.wrappedObjectType = 0xffFFffFFu;
			// will resolve this when we stop recording, in the hopes that if we
			// capture an entire frame, we will at some point have seen the owning
			// RT or DS.  If not, we may have to resort to using TrackedObjects to
			// hunt down the owner... (todo).
		}

		m_calls.push_back( call );
		return S_OK;
	}

	void	ResolveAliases()
	{
		for( size_t i = 0; i != m_calls.size(); ++i )
		{
			if( m_calls[i].m_call != Impl::SET_TEXTURE )
			{
				continue;
			}

			auto& param = m_calls[i].m_params.setTexture;

			// Try RTs first
			if( param.wrappedObjectAL == 0xffFFffFFu )
			{			
				for( size_t j = 0; j != m_rt.m_vec.size(); ++j )
				{
					auto& rt = *(Tr2RenderTargetAL*)(m_rt.m_vec[j].m_source);

					if( rt.IsValid() && (uint32_t)&rt.GetTexture() == param.texture )
					{
						param.wrappedObjectType = OT_RENDER_TARGET;
						param.wrappedObjectAL   = Tr2RenderContextAL::GetPrimaryRenderContext().IsBackBuffer( rt ) ? MAGIC_DEFAULT_BACKBUFFER : (uint32_t)j;
						param.texture           = 0xffFFffFFu;
						break;
					}
				}
			}

			// Try DSs first
			if( param.wrappedObjectAL == 0xffFFffFFu )
			{			
				for( size_t j = 0; j != m_ds.m_vec.size(); ++j )
				{
					auto& ds = *(Tr2DepthStencilAL*)m_ds.m_vec[j].m_source;

					if( ds.IsValid() && (uint32_t)&ds.GetTexture() == param.texture )
					{
						param.wrappedObjectType = OT_DEPTH_STENCIL;
						param.wrappedObjectAL   = (uint32_t)j;
						param.texture           = 0xffFFffFFu;
						break;
					}
				}
			}

			if( param.wrappedObjectAL == 0xffFFffFFu )
			{
				// not being updated in this frame... a copy should be fine
				Tr2TextureAL* source = (Tr2TextureAL*)param.texture;
				param.texture = m_tx.FindOrAdd( *source );
				param.wrappedObjectType = 0;
				param.wrappedObjectAL   = 0;
			}
		}
	}

	void	ExecuteDelayedClones()
	{
		m_cb.ExecuteDelayedClones();
		m_vb.ExecuteDelayedClones();
		m_ib.ExecuteDelayedClones();
		m_vl.ExecuteDelayedClones();
		m_sh.ExecuteDelayedClones();
		m_ds.ExecuteDelayedClones();
		m_rt.ExecuteDelayedClones();
		m_tx.ExecuteDelayedClones();
		m_ss.ExecuteDelayedClones();
	}

	ALResult DrawIndexedPrimitive(	uint32_t numVertices, 
									uint32_t startIndex, 
									uint32_t primitiveCount, 
									uint32_t minimumIndex ) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_INDEXED_PRIMITIVE;
		auto& param = call.m_params.drawPrimitive;
		memset( &param, 0, sizeof( param ) );

		param.numVertices		= numVertices;
		param.startIndex		= startIndex;
		param.primitiveCount	= primitiveCount;
		param.minimumIndex		= minimumIndex;

		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult DrawPrimitive(			uint32_t startVertex, 
									uint32_t primitiveCount ) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_PRIMITIVE;
		auto& param = call.m_params.drawPrimitive;
		memset( &param, 0, sizeof( param ) );

		param.startIndex		= startVertex;
		param.primitiveCount	= primitiveCount;
		
		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult DrawIndexedInstanced(	uint32_t numVertices, 
									uint32_t startIndex, 
									uint32_t primitiveCount, 
									uint32_t numInstances ) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_INDEXED_INSTANCED;
		auto& param = call.m_params.drawPrimitive;
		memset( &param, 0, sizeof( param ) );

		param.numVertices		= numVertices;
		param.startIndex		= startIndex;
		param.primitiveCount	= primitiveCount;
		param.numInstances		= numInstances;

		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride ) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_INDEXED_PRIMITIVE_UP;
		auto& param = call.m_params.drawPrimitiveUP;
		memset( &param, 0, sizeof( param ) );

		param.numVertices		= numVertices;
		param.primitiveCount	= primitiveCount;
		param.indexBuffer		= FindOrAdd( indexData, Tr2RenderContextAL::GetPrimaryRenderContext().ComputeVertexCount( primitiveCount ) * 4 );
		param.vertexBuffer		= FindOrAdd( vertexStreamZeroData, vertexStreamZeroStride );
		param.bitCount			= IB_32BIT;
		
		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_INDEXED_PRIMITIVE_UP;
		auto& param = call.m_params.drawPrimitiveUP;
		memset( &param, 0, sizeof( param ) );

		param.numVertices		= numVertices;
		param.primitiveCount	= primitiveCount;
		param.indexBuffer		= FindOrAdd( indexData, Tr2RenderContextAL::GetPrimaryRenderContext().ComputeVertexCount( primitiveCount ) * 4 );
		param.vertexBuffer		= FindOrAdd( vertexStreamZeroData, vertexStreamZeroStride );
		param.bitCount			= IB_16BIT;
		
		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult DrawPrimitiveUP(		
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride ) throw()
	{
		CapturedCall call;
		call.m_call = DRAW_PRIMITIVE_UP;
		auto& param = call.m_params.drawPrimitiveUP;
		memset( &param, 0, sizeof( param ) );

		param.primitiveCount	= primitiveCount;
		param.vertexBuffer		= FindOrAdd( vertexStreamZeroData, vertexStreamZeroStride );
		
		m_calls.push_back( call );

		ExecuteDelayedClones();
		return S_OK;
	}

	ALResult SetConstants(			
		const Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t unusedArgument = 0 ) throw()
	{
		CapturedCall call;
		call.m_call = SET_CONSTANTS;
		auto& param = call.m_params.setConstants;
		param.buffer			= m_cb.FindOrAdd( buffer );
		param.constantType		= constantType;
		param.registerIndex		= registerIndex;
		param.unusedArgument	= unusedArgument;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetRenderState(			
		Tr2RenderContextEnum::RenderState state, 
		uint32_t value ) throw()
	{
		CapturedCall call;
		call.m_call = SET_RENDER_STATE;
		call.m_params.misc.u32 = state;
		call.m_params.misc.u32_bis = value;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetRenderStates(		
		const uint32_t* stateValuePairs, 
		uint32_t count ) throw()
	{
		CapturedCall call;
		call.m_call = SET_RENDER_STATES;
		if( !count )
		{
			while( stateValuePairs[count*2] )
			{
				++count;
			}
		}
		call.m_params.misc.u32 = FindOrAdd( stateValuePairs, count * 2 * 4 );
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq ) throw()
	{
		CapturedCall call;
		call.m_call = SET_CLIP_PLANE;
		auto& p = call.m_params.setClipPlane;
		p.planeIndex = planeIndex;
		p.planeEq[0] = planeEq[0];
		p.planeEq[1] = planeEq[1];
		p.planeEq[2] = planeEq[2];
		p.planeEq[3] = planeEq[3];
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetScissorRect(			
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom ) throw()
	{
		CapturedCall call;
		call.m_call = SET_SCISSOR_RECT;
		auto& p = call.m_params.setScissorRect;
		p.left	= left;
		p.top	= top;
		p.right = right;		
		p.bottom= bottom;

		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber ) throw()
	{
		CapturedCall call;
		call.m_call = SET_SAMPLER_STATE;
		auto& param = call.m_params.setSamplerState;
		param.buffer			= m_ss.FindOrAdd( samplerState );
		param.inputType			= inputType;
		param.registerNumber	= registerNumber;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil ) throw()
	{
		CapturedCall call;
		call.m_call = SET_DEPTH_STENCIL;
		call.m_params.misc.objectAL = m_ds.FindOrAdd( depthStencil );
		
		m_calls.push_back( call );
		return S_OK;
	}

	void SetReadOnlyDepth( bool enable ) throw()
	{
		CapturedCall call;
		call.m_call = SET_READ_ONLY_DEPTH;
		call.m_params.misc.u32 = enable;
		
		m_calls.push_back( call );
	}

	ALResult SetRenderTarget( 
		const Tr2RenderTargetAL& renderTarget, 
		uint32_t slot = 0 ) throw()
	{
		CapturedCall call;
		call.m_call = SET_RENDER_TARGET;
		if( Tr2RenderContextAL::GetPrimaryRenderContext().IsBackBuffer( renderTarget ) )
		{
			call.m_params.misc.objectAL = MAGIC_DEFAULT_BACKBUFFER;
		}
		else
		{
			call.m_params.misc.objectAL	= m_rt.FindOrAdd( renderTarget );
		}
		call.m_params.misc.u32		= slot;
		
		m_calls.push_back( call );
		return S_OK;
	}
	

	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil )
	{
		CapturedCall call;
		call.m_call = CLEAR;
		auto& param = call.m_params.clear;

		param.clearFlags	= clearFlags;
		param.color			= color;
		param.depth			= depth;
		param.stencil		= stencil;
		
		m_calls.push_back( call );
		return S_OK;
	}

	void SetNumberOfLights( uint32_t numLights ) throw()
	{
		CapturedCall call;
		call.m_call = SET_NUMBER_OF_LIGHTS;
		call.m_params.misc.u32 = numLights;
		
		m_calls.push_back( call );
	}

	ALResult SetViewport( const Tr2Viewport& viewport ) throw()
	{
		CapturedCall call;
		call.m_call = SET_VIEWPORT;
		auto& p = call.m_params.setViewport;
		
		p.vp[0] = viewport.m_x;
		p.vp[1] = viewport.m_y;
		
		p.vp[2] = viewport.m_width;
		p.vp[3] = viewport.m_height;

		p.vp[4] = viewport.m_minZ;
		p.vp[5] = viewport.m_maxZ;
	
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult PushRenderTarget( uint32_t slot = 0 ) throw()
	{
		CapturedCall call;
		call.m_call = PUSH_RENDER_TARGET;
		call.m_params.misc.u32 = slot;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult PopRenderTarget( uint32_t slot = 0 ) throw()
	{
		CapturedCall call;
		call.m_call = POP_RENDER_TARGET;
		call.m_params.misc.u32 = slot;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult PushDepthStencil() throw()
	{
		CapturedCall call;
		call.m_call = PUSH_DEPTH_STENCIL;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult PopDepthStencil() throw()
	{
		CapturedCall call;
		call.m_call = POP_DEPTH_STENCIL;
		
		m_calls.push_back( call );
		return S_OK;
	}

	ALResult Resolve(	
		const Tr2RenderTargetAL& destination, 
		const Tr2RenderTargetAL& source ) throw()
	{
		auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
		CapturedCall call;
		call.m_call = RESOLVE;

		call.m_params.misc.objectAL = renderContext.IsBackBuffer( destination ) ? MAGIC_DEFAULT_BACKBUFFER : m_rt.FindOrAdd( destination );
		call.m_params.misc.u32      = renderContext.IsBackBuffer( source      ) ? MAGIC_DEFAULT_BACKBUFFER : m_rt.FindOrAdd( source );

		m_calls.push_back( call );
		return S_OK;
	}

	void Execute( size_t i )
	{
		auto& c = m_calls[i];
		auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
		switch( c.m_call )
		{
		case BEGIN_SCENE:	renderContext.BeginScene();		break;
		case END_SCENE:		renderContext.EndScene();		break;
	
		case SET_STREAM_SOURCE:
			{
				auto& p = c.m_params.setStreamSource;
				auto& clone = *m_vb.m_vec[p.buffer].m_clone;
				renderContext.SetStreamSource(	
						p.stream, 
						clone.IsValid() ? clone : nullVB,
						p.offset,
						p.stride );
				break;
			}

		case SET_INDICES:
			{
				auto& clone = *m_ib.m_vec[c.m_params.misc.objectAL].m_clone;
				renderContext.SetIndices( clone.IsValid() ? clone : nullIB );
			}
			break;

		case SET_TOPOLOGY:
			renderContext.SetTopology( static_cast<Topology>( c.m_params.misc.u32 ) );
			break;

		case SET_VERTEX_LAYOUT:
			renderContext.SetVertexLayout( *m_vl.m_vec[c.m_params.misc.objectAL].m_clone );
			break;

		case SET_SHADER:
			{
				auto& clone = *m_sh.m_vec[c.m_params.misc.objectAL].m_clone;
				renderContext.SetShader( clone.IsValid() ? clone : nullShader[ c.m_params.misc.u32 ] );				
			}
			break;

		case SET_SHADER_BUFFER:
			{
				auto& p = c.m_params.setShaderBuffer;
				renderContext.SetShaderBuffer(	p.inputType,
												p.slot,
												*m_uav[p.buffer].m_uav );
				break;
			}

		case SET_TEXTURE:
			{
				auto& p = c.m_params.setTexture;
				
				if( p.wrappedObjectAL == 0xffFFffFFu )	// unresolved alias?
				{
					break;
				}
				if( p.wrappedObjectAL == MAGIC_DEFAULT_BACKBUFFER )
				{
					renderContext.SetTexture(	p.inputType,
												p.slot,
												renderContext.GetDefaultBackBuffer().GetTexture(),
												p.colorSpace );
					break;
				}
				if( p.texture == 0xffFFffFFu )	// proper alias
				{
					Tr2TextureAL& texture = 
						p.wrappedObjectType == OT_DEPTH_STENCIL
							?	m_ds.m_vec[p.wrappedObjectAL].m_clone->GetTexture()
							:	m_rt.m_vec[p.wrappedObjectAL].m_clone->GetTexture();

					renderContext.SetTexture(	p.inputType,
												p.slot,
												texture,
												p.colorSpace );
				}
				else
				{
					auto& clone = *m_tx.m_vec[p.texture].m_clone;
					renderContext.SetTexture(	p.inputType,
												p.slot,
												clone.IsValid() ? clone : nullTX,
												p.colorSpace );
				}
				break;
			}

		case DRAW_INDEXED_PRIMITIVE:
			{
				auto& p = c.m_params.drawPrimitive;
				renderContext.DrawIndexedPrimitive(	p.numVertices,
													p.startIndex,
													p.primitiveCount,
													p.minimumIndex );
				break;
			}

		case DRAW_PRIMITIVE:
			{
				auto& p = c.m_params.drawPrimitive;
				renderContext.DrawPrimitive( p.startIndex,
											p.primitiveCount );
				break;
			}
		case DRAW_INDEXED_INSTANCED:
			{
				auto& p = c.m_params.drawPrimitive;
				renderContext.DrawIndexedInstanced(	p.numVertices,
													p.startIndex,
													p.primitiveCount,
													p.numInstances );
				break;
			}

		case DRAW_INDEXED_PRIMITIVE_UP:
			{
				auto& p = c.m_params.drawPrimitiveUP;
				if( p.bitCount == IB_32BIT )
				{
					renderContext.DrawIndexedPrimitiveUP(
						p.numVertices,
						p.primitiveCount,
						(const uint32_t*)m_rb[p.indexBuffer].m_copy.get(),
						m_rb[p.vertexBuffer].m_copy.get(),
						(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );
				}
				else
				{
					renderContext.DrawIndexedPrimitiveUP(
						p.numVertices,
						p.primitiveCount,
						(const uint16_t*)m_rb[p.indexBuffer].m_copy.get(),
						m_rb[p.vertexBuffer].m_copy.get(),
						(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );
				}
				break;
			}

		case DRAW_PRIMITIVE_UP:
			{
				auto& p = c.m_params.drawPrimitiveUP;
				renderContext.DrawPrimitiveUP(
						p.primitiveCount,
						m_rb[p.vertexBuffer].m_copy.get(),
						(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );				
				break;
			}

		case SET_RENDER_STATE:
			renderContext.SetRenderState( RenderState( c.m_params.misc.u32 ), c.m_params.misc.u32_bis );
			break;

		case SET_RENDER_STATES:
			renderContext.SetRenderStates(
				(const uint32_t*)m_rb[c.m_params.misc.u32].m_copy.get(), 
				(uint32_t)m_rb[c.m_params.misc.u32].m_copy.size() / ( 2 * 4 ) );
			break;

		case SET_CLIP_PLANE:
			{
				auto& p = c.m_params.setClipPlane;
				renderContext.SetClipPlane( p.planeIndex, p.planeEq );
				break;
			}

		case SET_SCISSOR_RECT:
			{
				auto& p = c.m_params.setScissorRect;
				renderContext.SetScissorRect( p.left, p.top, p.right, p.bottom );
				break;
			}

		case SET_SAMPLER_STATE:
			{
				auto& p = c.m_params.setSamplerState;
				renderContext.SetSamplerState(
					*m_ss.m_vec[p.buffer].m_clone,
					p.inputType,
					p.registerNumber );
				break;
			}

		case SET_CONSTANTS:
			{
				auto& p = c.m_params.setConstants;
				auto& clone = *m_cb.m_vec[p.buffer].m_clone;
				renderContext.SetConstants(
						clone.IsValid() ? clone : nullCB,
						p.constantType,
						p.registerIndex,
						p.unusedArgument );
				break;
			}

		case SET_DEPTH_STENCIL:
			{
				auto& clone = *m_ds.m_vec[c.m_params.misc.objectAL].m_clone;
				renderContext.SetDepthStencil( clone.IsValid() ? clone : nullDS );
			}
			break;

		case SET_READ_ONLY_DEPTH:
			renderContext.SetReadOnlyDepth( c.m_params.misc.u32 != 0 );
			break;

		case SET_RENDER_TARGET:
			{
				if( c.m_params.misc.objectAL == MAGIC_DEFAULT_BACKBUFFER )
				{
					renderContext.SetRenderTarget( nullRT );
				}
				else
				{
					auto& clone = *m_rt.m_vec[c.m_params.misc.objectAL].m_clone;
					renderContext.SetRenderTarget( clone.IsValid() ? clone : nullRT, c.m_params.misc.u32 );
				}
			}
			break;

		case CLEAR:
			{
				auto& p = c.m_params.clear;
				renderContext.Clear( p.clearFlags, p.color, p.depth, p.stencil );
				break;
			}

		case SET_NUMBER_OF_LIGHTS:
			renderContext.SetNumberOfLights( c.m_params.misc.u32 );
			break;

		case SET_VIEWPORT:
			{
				Tr2Viewport vp;
				auto& p = c.m_params.setViewport;
				vp.m_x		= p.vp[0];
				vp.m_y		= p.vp[1];
				vp.m_width	= p.vp[2];
				vp.m_height = p.vp[3];
				vp.m_minZ	= p.vp[4];
				vp.m_maxZ	= p.vp[5];
				renderContext.SetViewport( vp );
				break;
			}

		case PUSH_RENDER_TARGET:
			renderContext.PushRenderTarget( c.m_params.misc.u32 );
			break;

		case POP_RENDER_TARGET:
			renderContext.PopRenderTarget( c.m_params.misc.u32 );
			break;

		case PUSH_DEPTH_STENCIL:
			renderContext.PushDepthStencil();
			break;

		case POP_DEPTH_STENCIL:
			renderContext.PopDepthStencil();
			break;

		case RESOLVE:
			{
				Tr2RenderTargetAL* src = 
					c.m_params.misc.u32 == MAGIC_DEFAULT_BACKBUFFER 
						? &renderContext.GetDefaultBackBuffer()
						: m_rt.m_vec[ c.m_params.misc.u32 ].m_clone.get();
			
				Tr2RenderTargetAL* dst = 
					c.m_params.misc.objectAL == MAGIC_DEFAULT_BACKBUFFER 
						? &renderContext.GetDefaultBackBuffer()
						: m_rt.m_vec[ c.m_params.misc.objectAL ].m_clone.get();

				src->Resolve( *dst, renderContext );
			}			
			break;

		default:
			CCP_ASSERT( false );
		}
	}

	void Save( std::string folder )
	{
		if( folder.empty() || ( folder.back() != '/' && folder.back() != '\\' ) )
		{
			folder.push_back( '/' );
		}

		static const char* const friendlyType[ INVALID_SHADER ] =
		{
			"VERTEX_SHADER",
			"PIXEL_SHADER",
			"COMPUTE_SHADER",
			"GEOMETRY_SHADER",
			"HULL_SHADER",
			"DOMAIN_SHADER",
		};

		static const char* const friendlyName[ INVALID_CALL ] =
		{	
			"BEGIN_SCENE",
			"END_SCENE",
	
			"SET_STREAM_SOURCE",
			"SET_INDICES",
			"SET_TOPOLOGY",
			"SET_VERTEX_LAYOUT",
			"SET_SHADER",

			"SET_SHADER_BUFFER",
			"SET_TEXTURE",

			"DRAW_INDEXED_PRIMITIVE",
			"DRAW_PRIMITIVE",
			"DRAW_INDEXED_INSTANCED",

			"DRAW_INDEXED_PRIMITIVE_UP",
			"DRAW_PRIMITIVE_UP",

			"SET_RENDER_STATE",
			"SET_RENDER_STATES",

			"SET_CLIP_PLANE",
			"SET_SCISSOR_RECT",
			"SET_SAMPLER_STATE",

			"SET_CONSTANTS",

			"SET_DEPTH_STENCIL",
			"SET_READ_ONLY_DEPTH",
			"SET_RENDER_TARGET",

			"CLEAR",

			"SET_NUMBER_OF_LIGHTS",
			"SET_VIEWPORT",

			"PUSH_RENDER_TARGET",
			"POP_RENDER_TARGET",

			"PUSH_DEPTH_STENCIL",
			"POP_DEPTH_STENCIL",

			"RESOLVE"
		};

		std::ofstream main( ( folder + "capture.yaml" ).c_str() );

		main	<< "\n\n# RENDERING CALLS --------------------\n"
				<< "calls:\n";

		size_t numDrawcall = 0;

		for( size_t i = 0; i != m_calls.size(); ++i )
		{
			const auto& c = m_calls[i];

			main	<< "\n# [" << std::setw( 4 ) << std::setfill( '0' ) << i << "] " << friendlyName[c.m_call] << '\n'
					<< " - call: " << c.m_call << '\n';


			switch( c.m_call )
			{
			case BEGIN_SCENE:	break;
			case END_SCENE:		break;

			case SET_STREAM_SOURCE:
				{					
					auto& p = c.m_params.setStreamSource;
					auto& clone = *m_vb.m_vec[p.buffer].m_clone;

					main	<< "   params:\n"
							<< "     " << "stream: " << p.stream << '\n'
							<< "     " << "offset: " << p.offset << '\n'
							<< "     " << "stride: " << p.stride << '\n'
							<< "     " << "vb:     " << int( clone.IsValid() ? p.buffer : -1 ) << '\n';
					break;
				}

			case SET_INDICES:
				{
					auto& clone = *m_ib.m_vec[c.m_params.misc.objectAL].m_clone;
					main	<< "  params:\n"
							<< "    " << "ib:     " << int( clone.IsValid() ? c.m_params.misc.objectAL : -1 ) << '\n';
				}
				break;

			case SET_TOPOLOGY:
				{
					static const char* friendly[TOP_MAX_TOPOLOGY] = {
						"TOP_INVALID",
						"TOP_TRIANGLES",
						"TOP_TRIANGLE_STRIP",
						"TOP_TRIANGLE_FAN",
						"TOP_LINES",
						"TOP_LINE_STRIP",
						"TOP_POINTS"
					};
					main	<< "   params:\n"
							<< "     " << "topology: " << c.m_params.misc.u32
							<< " # " << friendly[c.m_params.misc.u32] << '\n';
				}
				break;

			case SET_VERTEX_LAYOUT:
				main	<< "   params:\n"
							<< "     " << "layout: " << c.m_params.misc.objectAL << '\n';
				break;

			case SET_SHADER:
				{
					auto& clone = *m_sh.m_vec[c.m_params.misc.objectAL].m_clone;
					main	<< "   params:\n"
							<< "     " << "shader:   " << int( clone.IsValid() ? c.m_params.misc.objectAL : -1 ) << '\n'
							<< "     " << "type:     " << c.m_params.misc.u32 << '\n';					
				}
				break;

			case SET_SHADER_BUFFER:
				{
					auto& p = c.m_params.setShaderBuffer;
					main	<< "   params:\n"
							<< "     " << "inputType: " << p.inputType << " # " << friendlyType[ p.inputType ] << '\n'
							<< "     " << "slot:      " << p.slot      << '\n'
							<< "     " << "uav:       " << p.buffer    << '\n';
					break;
				}

			case SET_TEXTURE:
				{
					auto& p = c.m_params.setTexture;

					main	<< "   params:\n"
							<< "     " << "inputType:         " << p.inputType         << " # " << friendlyType[ p.inputType ] << '\n'
							<< "     " << "slot:              " << p.slot              << '\n'
							<< "     " << "colorSpace:        " << p.colorSpace        << '\n'
							<< "     " << "texture:           " << p.texture           << '\n'
							<< "     " << "wrappedObjectAL:   " << p.wrappedObjectAL   << '\n'
							<< "     " << "wrappedObjectType: " << p.wrappedObjectType << '\n'
							<< "     # ";
					
					if( p.wrappedObjectAL == 0xffFFffFFu )	// unresolved alias?
					{
						main << "unresolved alias\n";
						break;
					}
					if( p.texture == 0xffFFffFFu )	// proper alias
					{
						if( p.wrappedObjectType == OT_DEPTH_STENCIL )
						{
							main << "depthStencil backend\n";
						}
						else
						{
							main << "renderTarget backend \n";
						}	
					}
					else
					{						
						auto& clone = *m_tx.m_vec[p.texture].m_clone;
						if( !clone.IsValid() )
						{
							main << "nullTX\n";
						}
						else
						{
							main << "regular texture, saved as 'capture" << std::setfill( '0' ) << std::setw( 5 ) << p.texture << ".dds' \n";
						}
					}
					break;
				}

			case DRAW_INDEXED_PRIMITIVE:
				{
					++numDrawcall;
					auto& p = c.m_params.drawPrimitive;
					main	<< "   params:\n"
							<< "     " << "numVertices:    " << p.numVertices    << '\n'
							<< "     " << "startIndex:     " << p.startIndex     << '\n'
							<< "     " << "primitiveCount: " << p.primitiveCount << '\n'
							<< "     " << "minimumIndex:   " << p.minimumIndex   << '\n';
					break;
				}

			case DRAW_PRIMITIVE:
				{
					++numDrawcall;
					auto& p = c.m_params.drawPrimitive;
					main	<< "   params:\n"
							<< "     " << "startIndex:     " << p.startIndex     << '\n'
							<< "     " << "primitiveCount: " << p.primitiveCount << '\n';					
					break;
				}
			case DRAW_INDEXED_INSTANCED:
				{
					++numDrawcall;
					auto& p = c.m_params.drawPrimitive;
					main	<< "   params:\n"
							<< "     " << "numVertices:    " << p.numVertices    << '\n'
							<< "     " << "startIndex:     " << p.startIndex     << '\n'
							<< "     " << "primitiveCount: " << p.primitiveCount << '\n'
							<< "     " << "numInstances:   " << p.numInstances   << '\n';					
					break;
				}
#if 0
			case DRAW_INDEXED_PRIMITIVE_UP:
				{
					auto& p = c.m_params.drawPrimitiveUP;
					if( p.bitCount == IB_32BIT )
					{
						renderContext.DrawIndexedPrimitiveUP(
							p.numVertices,
							p.primitiveCount,
							(const uint32_t*)m_rb[p.indexBuffer].m_copy.get(),
							m_rb[p.vertexBuffer].m_copy.get(),
							(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );
					}
					else
					{
						renderContext.DrawIndexedPrimitiveUP(
							p.numVertices,
							p.primitiveCount,
							(const uint16_t*)m_rb[p.indexBuffer].m_copy.get(),
							m_rb[p.vertexBuffer].m_copy.get(),
							(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );
					}
					break;
				}

			case DRAW_PRIMITIVE_UP:
				{
					auto& p = c.m_params.drawPrimitiveUP;
					renderContext.DrawPrimitiveUP(
						p.primitiveCount,
						m_rb[p.vertexBuffer].m_copy.get(),
						(uint32_t)m_rb[p.vertexBuffer].m_copy.size() );				
					break;
				}
#endif
			case SET_RENDER_STATE:
					main	<< "   params:\n"
							<< "     " << "state: " << c.m_params.misc.u32 << '\n'
							<< "     " << "value: " << c.m_params.misc.u32_bis << '\n';
				break;

			case SET_RENDER_STATES:
				{
					main	<< "   params:\n"
							<< "     keyValues: [ ";

					const uint32_t count      = (uint32_t)m_rb[c.m_params.misc.u32].m_copy.size() / ( 2 * 4 );
					const uint32_t *kv = (const uint32_t*)m_rb[c.m_params.misc.u32].m_copy.get();

					for( size_t i = 0; i != count; ++i )
					{
						main << "( " << kv[i*2+0] << ", " << kv[i*2+1] << " ) ";
						if( i+1 != count )
						{
							main << ", ";
						}
					}

					main	<< "]\n";				
				}
				break;

			case SET_CLIP_PLANE:
				{
					auto& p = c.m_params.setClipPlane;
					main	<< "   params:\n"
							<< "     " << "index: [ " << p.planeIndex << '\n'
							<< "     " << "vec4: ( " << p.planeEq[0] << " , " << p.planeEq[1] << " , " << p.planeEq[2] << " , " << p.planeEq[3] << " )\n";							
					break;
				}

			case SET_SCISSOR_RECT:
				{
					auto& p = c.m_params.setScissorRect;
					main	<< "   params:\n"
							<< "     " << "left:   " << p.left   << '\n'
							<< "     " << "top:    " << p.top    << '\n'
							<< "     " << "right:  " << p.right  << '\n'
							<< "     " << "bottom: " << p.bottom << '\n';					
					break;
				}

			case SET_SAMPLER_STATE:
				{
					auto& p = c.m_params.setSamplerState;
					main	<< "   params:\n"
							<< "     " << "inputType:      " << p.inputType      << " # " << friendlyType[ p.inputType ] << '\n'
							<< "     " << "registerNumber: " << p.registerNumber << '\n'
							<< "     " << "ss:             " << p.buffer         << '\n';					
					break;
				}

			case SET_CONSTANTS:
				{
					auto& p = c.m_params.setConstants;
					auto& clone = *m_cb.m_vec[p.buffer].m_clone;

					main	<< "   params:\n"
							<< "     " << "constantType:   " << p.constantType   << " # " << friendlyType[ p.constantType ] << '\n'
							<< "     " << "registerIndex:  " << p.registerIndex  << '\n'
							<< "     " << "unusedArgument: " << p.unusedArgument << '\n'
							<< "     " << "cb:             " << int( clone.IsValid() ? p.buffer : -1 ) << '\n';					
					break;
				}

			case SET_DEPTH_STENCIL:
				{
					auto& clone = *m_ds.m_vec[c.m_params.misc.objectAL].m_clone;
					main	<< "   params:\n"
							<< "     " << "ds: " << int( clone.IsValid() ? c.m_params.misc.objectAL : -1 ) << '\n';					
				}
				break;

			case SET_READ_ONLY_DEPTH:
					main	<< "   params:\n"
							<< "     " << "readOnly: " << c.m_params.misc.u32 << '\n';							
				break;

			case SET_RENDER_TARGET:				
				{					
					main	<< "   params:\n"
							<< "     " << "slot: " << c.m_params.misc.u32 << '\n';
					if( c.m_params.misc.objectAL == MAGIC_DEFAULT_BACKBUFFER )
					{
						main<< "     " << "rt:   backbuffer\n";
					}
					else
					{
						auto& clone = *m_rt.m_vec[c.m_params.misc.objectAL].m_clone;
						main<< "     " << "rt:   " << int( clone.IsValid() ? c.m_params.misc.objectAL : -1 ) << '\n';
					}
				}
				break;

			case CLEAR:
				{
					auto& p = c.m_params.clear;
					main	<< "   params:\n"
							<< "     " << "clearFlags: " << p.clearFlags << " # ( "
							<< ( p.clearFlags & CLEARFLAGS_TARGET  ? "CLEARFLAGS_TARGET "  : "" )
							<< ( p.clearFlags & CLEARFLAGS_ZBUFFER ? "CLEARFLAGS_ZBUFFER " : "" )
							<< ( p.clearFlags & CLEARFLAGS_STENCIL ? "CLEARFLAGS_STENCIL " : "" )
							<< " )\n"
							<< "     " << "color:      " << p.color      << " # ( "														
							<< uint32_t( ( p.color >> 24 ) & 255 ) << ", "
							<< uint32_t( ( p.color >> 16 ) & 255 ) << ", "							
							<< uint32_t( ( p.color >> 8 ) & 255 )  << ", "
							<< uint32_t( p.color & 255 )
							<< " )\n"
							<< "     " << "depth:      " << p.depth      << '\n'
							<< "     " << "stencil:    " << p.stencil    << '\n';
					break;
				}

			case SET_NUMBER_OF_LIGHTS:
					main	<< "   params:\n"
							<< "     " << "numberOfLights: " << c.m_params.misc.u32 << '\n';							
				break;

			case SET_VIEWPORT:
				{
					auto& vp = c.m_params.setViewport.vp;
					main	<< "   params:\n"
							<< "     " << "x:      " << vp[0] << '\n'
							<< "     " << "y:      " << vp[1] << '\n'
							<< "     " << "width:  " << vp[2] << '\n'
							<< "     " << "height: " << vp[3] << '\n'
							<< "     " << "minZ:   " << vp[4] << '\n'
							<< "     " << "maxZ:   " << vp[5] << '\n';					
					break;
				}

			case PUSH_RENDER_TARGET:
					main	<< "   params:\n"
							<< "     " << "slot: " << c.m_params.misc.u32 << '\n';												
				break;

			case POP_RENDER_TARGET:
					main	<< "   params:\n"
							<< "     " << "slot: " << c.m_params.misc.u32 << '\n';							
				break;

			case PUSH_DEPTH_STENCIL:
				break;

			case POP_DEPTH_STENCIL:
				break;
			
			case RESOLVE:
					main	<< "   params:\n"
							<< "     " << "destination: " << c.m_params.misc.objectAL << '\n'
							<< "     " << "source:      " << c.m_params.misc.u32      << '\n'
							<< "     " << "# ( "
							<< ( c.m_params.misc.objectAL == MAGIC_DEFAULT_BACKBUFFER ? "backbuffer" : "renderTarget" )
							<< " <-- "
							<< ( c.m_params.misc.u32      == MAGIC_DEFAULT_BACKBUFFER ? "backbuffer" : "renderTarget" )
							<< " )\n";
				break;

			default:
				CCP_ASSERT( false );
			}
		}

		main << "\n\n# TEXTURES --------------------\n"
			 << "textures:\n";

		size_t textureEstimate = 0;

		for( size_t i = 0; i != m_tx.m_vec.size(); ++i )
		{
			auto& it = m_tx.m_vec[i];

			main	<< "\n   # [" << std::setw( 4 ) << std::setfill( '0' ) << i << "] " << '\n'
					<< " - index: " << i << '\n';

			auto& tx  = *it.m_clone;			
			Tr2TextureAL* src = (Tr2TextureAL*)it.m_source;

			if( !tx.IsValid() || !src )
			{
				continue;
			}

			if( src->IsAlias() )
			{
				main << "# alias for RT/DS\n";
			}
			else
			{
				main	<< "   params:\n"
						<< "     " << "source: " << (uint32_t)it.m_source	<< '\n'
						<< "     " << "type:   " << src->GetType()			<< '\n'
						<< "     " << "format: " << src->GetFormat()		<< '\n'
						<< "     " << "width:  " << src->GetWidth()		<< '\n'
						<< "     " << "height: " << src->GetHeight()		<< '\n'						
						<< "     " << "depth:  " << src->GetDepth()		<< '\n'
						<< "     " << "mips:   " << src->GetMipCount()		<< '\n';						
			}
					
			if( !src->IsAlias() )
			{
				char buf[256];
				_snprintf_s( buf, 256, "%scapture%05d.dds", folder.c_str(), i );
				if( SaveTextureToDds( tx, CA2W( buf ) ) )
				{
					main << "     # saved as " << buf << '\n';
				}

				for( uint32_t level = 0; level != src->GetTrueMipCount(); ++level )
				{
					textureEstimate += src->GetMipSize( level );
				}
			}
		}

		main	<< "\n\n# CONSTANT BUFFERS --------------------\n"
				<< "constantBuffers:\n";

		for( size_t i = 0; i != m_cb.m_vec.size(); ++i )
		{
			auto& it = m_cb.m_vec[i];

			main	<< "\n# [" << std::setw( 4 ) << std::setfill( '0' ) << i << "] " << '\n'
					<< " - index: " << i << '\n';

			if( !it.m_source || !it.m_clone || !it.m_clone->IsValid() )
			{
				continue;
			}

			main	<< "   params:\n"
					<< "     " << "source: " << (uint32_t)it.m_source	<< '\n'
					<< "     " << "size:   " << it.m_clone->GetSize() << '\n';
		}

		main	<< "\n\n# VERTEX BUFFERS --------------------\n"
				<< "vertexBuffers:\n";

		size_t vbEstimate = 0;
		std::set<const void*> vbSeen;

		for( size_t i = 0; i != m_vb.m_vec.size(); ++i )
		{
			auto& it = m_vb.m_vec[i];

			main	<< "\n# [" << std::setw( 4 ) << std::setfill( '0' ) << i << "] " << '\n'
					<< " - index: " << i << '\n';

			if( !it.m_source || !it.m_clone || !it.m_clone->IsValid() )
			{
				continue;
			}

			main	<< "   params:\n"
					<< "     " << "source: "  << (uint32_t)it.m_source	<< '\n'
					<< "     " << "size:   "  << it.m_clone->GetTotalSizeInBytes() << '\n'
					<< "     " << "usage:   " << it.m_clone->m_usage << '\n';

			if( vbSeen.insert( it.m_source ).second )
			{
				vbEstimate += it.m_clone->GetTotalSizeInBytes();
			}
		}

		main	<< "\n\n# INDEX BUFFERS --------------------\n"
				<< "indexBuffers:\n";

		size_t ibEstimate = 0;
		std::set<const void*> ibSeen;

		for( size_t i = 0; i != m_ib.m_vec.size(); ++i )
		{
			auto& it = m_ib.m_vec[i];

			main	<< "\n# [" << std::setw( 4 ) << std::setfill( '0' ) << i << "] " << '\n'
					<< " - index: " << i << '\n';

			if( !it.m_source || !it.m_clone || !it.m_clone->IsValid() )
			{
				continue;
			}

			main	<< "   params:\n"
					<< "     " << "source:          "  << (uint32_t)it.m_source	<< '\n'
					<< "     " << "bytesPerIndex:   "  << it.m_clone->BytesPerIndex()		<< '\n'
					<< "     " << "numIndices:      "  << it.m_clone->GetNumIndices()		<< '\n';

			if( ibSeen.insert( it.m_source ).second )
			{
				ibEstimate += it.m_clone->GetTotalSizeInBytes();
			}			
		}

		main	<< "\n\n# STATISTICS --------------------\n"
				<< std::setfill(' ')
				<< "stats:\n"
				<< "  textureEstimate: " << std::setw( 10 ) << textureEstimate << "    # bytes \n"
				<< "  vbEstimate:      " << std::setw( 10 ) << vbEstimate      << "    # bytes \n"
				<< "  ibEstimate:      " << std::setw( 10 ) << ibEstimate      << "    # bytes \n\n"

				<< "  apiCalls:        " << std::setw( 10 ) << m_calls.size()  << "    # counter \n"
				<< "  drawCalls:       " << std::setw( 10 ) << numDrawcall     << "    # counter \n\n"

				<< "  numVB:           " << std::setw( 10 ) << m_vb.CountUnique()  << "    # vertex buffers \n"
				<< "  numIB:           " << std::setw( 10 ) << m_ib.CountUnique()  << "    # index buffers \n"
				<< "  numCB:           " << std::setw( 10 ) << m_cb.CountUnique()  << "    # constant buffers \n"
				<< "  numRT:           " << std::setw( 10 ) << m_rt.CountUnique()  << "    # renderTargets \n"
				<< "  numDS:           " << std::setw( 10 ) << m_ds.CountUnique()  << "    # depthStencils \n"
				<< "  numSH:           " << std::setw( 10 ) << m_sh.CountUnique()  << "    # shaders \n"
				<< "  numSS:           " << std::setw( 10 ) << m_ss.CountUnique()  << "    # samplerStates \n"
				<< "  numTX:           " << std::setw( 10 ) << m_tx.CountUnique()  << "    # textures \n"
				<< "  numVL:           " << std::setw( 10 ) << m_vl.CountUnique()  << "    # vertexLayouts \n";
	}

	void PlayFrame()
	{
		for( size_t i = 0; i != m_calls.size(); ++i )
		{
			Execute( i );
		}
	}
};

Tr2RenderCapture::Tr2RenderCapture()
	: m_state( INACTIVE )
	, m_impl( new Impl )
{
}

Tr2RenderCapture::~Tr2RenderCapture()
{
}

void Tr2RenderCapture::PresentHook()
{
	// We use the Present() call as the end/begin of a frame to capture.
	// If we're waiting, start recording.  If we're recording, stop.
	// If we're playing, keep going.

	switch( m_state )
	{
	case WAITING:
		StartRecording();
		return;

	case RECORDING:
		StopRecording();
		return;

	case PLAYING:
		if( !g_trinityCapturePlay )
		{
			StopPlaying();
			return;
		}
		PlayFrame();		
		return;
	
	case INACTIVE:
		if( g_trinityCaptureRecord )
		{
			StartRecording();
			g_trinityCaptureRecord = false;
			return;
		}

		if( g_trinityCapturePlay )
		{
			StartPlaying();
			return;
		}
	}
}

void Tr2RenderCapture::PlayFrame()
{
	auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();

	// reset m_esm, otherwise it may think that certain textures are already bound, and not do anything
	// during playback, even though the actual AL calls were blocked.

	//renderContext.m_esm.BeginManagedRendering();

	// make the calls actually do anything
	m_state = INACTIVE;	

	// go back to blocking calls
	ON_BLOCK_EXIT( [&]{ m_state = PLAYING; } );
	
	// clear current RT as a test
	renderContext.Clear(	CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER | CLEARFLAGS_STENCIL,
							0x00ff00ff, 1.0f, 0 );
	
	m_impl->PlayFrame();

	//renderContext.m_esm.EndManagedRendering();
}

void Tr2RenderCapture::StartRecording()
{
	m_impl->Clear();
	m_state = RECORDING;
}

void Tr2RenderCapture::StopRecording()
{
	m_state = INACTIVE;

	m_impl->ResolveAliases();
}

void Tr2RenderCapture::StartPlaying()
{
	m_state = PLAYING;

	auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
	renderContext.ResetCapturePlayback();
}

void Tr2RenderCapture::StopPlaying()
{
	m_state = INACTIVE;
		
	auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
	//renderContext.m_esm.BeginManagedRendering();
	//renderContext.m_esm.EndManagedRendering();
	renderContext.ResetCapturePlayback();
}

ALResult Tr2RenderCapture::SetStreamSource(		
	uint32_t stream, 
	const Tr2VertexBufferAL & buffer, 
	uint32_t offset, 
	uint32_t stride )
{
	return m_impl->SetStreamSource( stream, buffer, offset, stride );
}

ALResult Tr2RenderCapture::SetIndices( const Tr2IndexBufferAL & buffer )
{
	return m_impl->SetIndices( buffer );
}

ALResult Tr2RenderCapture::SetTopology( Tr2RenderContextEnum::Topology topology )
{
	return m_impl->SetTopology( topology );
}
	
ALResult Tr2RenderCapture::SetVertexLayout( Tr2VertexLayoutAL& layout )
{
	return m_impl->SetVertexLayout( layout );
}
	
ALResult Tr2RenderCapture::SetShader( const Tr2ShaderAL& shader )
{
	return m_impl->SetShader( shader );
}

ALResult Tr2RenderCapture::SetShaderBuffer(	
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t slot, 
	const Tr2GpuBufferAL& buffer )
{
	return m_impl->SetShaderBuffer( inputType, slot, buffer );
}

ALResult Tr2RenderCapture::SetTexture(	
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t slot, 
	const Tr2TextureAL& texture, 
	Tr2RenderContextEnum::ColorSpace colorSpace )
{
	return m_impl->SetTexture( inputType, slot, texture, colorSpace );
}

ALResult Tr2RenderCapture::DrawIndexedPrimitive(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t minimumIndex ) throw()
{
	return m_impl->DrawIndexedPrimitive( numVertices, startIndex, primitiveCount, minimumIndex );
}

ALResult Tr2RenderCapture::DrawPrimitive(			
	uint32_t startVertex, 
	uint32_t primitiveCount ) throw()
{
	return m_impl->DrawPrimitive( startVertex, primitiveCount );
}

ALResult Tr2RenderCapture::DrawIndexedInstanced(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t numInstances ) throw()
{
	return m_impl->DrawIndexedInstanced( numVertices, startIndex, primitiveCount, numInstances );
}

ALResult Tr2RenderCapture::DrawIndexedPrimitiveUP( 
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride ) throw()
{
	return m_impl->DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride );
}

ALResult Tr2RenderCapture::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride) throw()
{
	return m_impl->DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride );
}

ALResult Tr2RenderCapture::DrawPrimitiveUP(		
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride ) throw()
{
	return m_impl->DrawPrimitiveUP(	primitiveCount,
									vertexStreamZeroData,
									vertexStreamZeroStride );
}

ALResult Tr2RenderCapture::SetConstants(		
	const Tr2ConstantBufferAL& buffer, 
	Tr2RenderContextEnum::ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t unusedArgument ) throw()
{
	return m_impl->SetConstants( buffer, constantType, registerIndex, unusedArgument );
}

ALResult Tr2RenderCapture::SetRenderState(		
	Tr2RenderContextEnum::RenderState state, 
	uint32_t value ) throw()
{
	return m_impl->SetRenderState( state, value );
}

ALResult Tr2RenderCapture::SetRenderStates(		
	const uint32_t* stateValuePairs, 
	uint32_t count ) throw()
{
	return m_impl->SetRenderStates( stateValuePairs, count );
}

ALResult Tr2RenderCapture::SetClipPlane(		
	uint32_t planeIndex, 
	const float* planeEq ) throw()
{
	return m_impl->SetClipPlane( planeIndex, planeEq );
}

ALResult Tr2RenderCapture::SetScissorRect(		
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom ) throw()
{
	return m_impl->SetScissorRect( left, top, right, bottom );
}

ALResult Tr2RenderCapture::SetSamplerState(		
	const Tr2SamplerStateAL& samplerState, 
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t registerNumber ) throw()
{
	return m_impl->SetSamplerState( samplerState, inputType, registerNumber );
}

ALResult Tr2RenderCapture::SetDepthStencil( const Tr2DepthStencilAL& depthStencil ) throw()
{
	return m_impl->SetDepthStencil( depthStencil );
}
	
void Tr2RenderCapture::SetReadOnlyDepth( bool enable ) throw()
{
	return m_impl->SetReadOnlyDepth( enable );
}

ALResult Tr2RenderCapture::SetRenderTarget(	const Tr2RenderTargetAL& renderTarget, 
											uint32_t slot ) throw()
{
	return m_impl->SetRenderTarget( renderTarget, slot );
}

ALResult Tr2RenderCapture::Clear(
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil ) throw()
{
	return m_impl->Clear( clearFlags, color, depth, stencil );
}

void Tr2RenderCapture::SetNumberOfLights(	uint32_t numLights ) throw()
{
	return m_impl->SetNumberOfLights( numLights );
}

ALResult Tr2RenderCapture::SetViewport( const Tr2Viewport& viewport ) throw()
{
	return m_impl->SetViewport( viewport );
}

ALResult Tr2RenderCapture::PushRenderTarget( uint32_t slot ) throw()
{
	return m_impl->PushRenderTarget( slot );
}

ALResult Tr2RenderCapture::PopRenderTarget( uint32_t slot ) throw()
{
	return m_impl->PopRenderTarget( slot );
}

ALResult Tr2RenderCapture::PushDepthStencil() throw()
{
	return m_impl->PushDepthStencil();
}

ALResult Tr2RenderCapture::PopDepthStencil() throw()
{
	return m_impl->PopDepthStencil();
}

ALResult Tr2RenderCapture::Resolve( 
	const Tr2RenderTargetAL& destination, 
	const Tr2RenderTargetAL& source ) throw()
{
	return m_impl->Resolve( destination, source );
}

void Tr2RenderCapture::Save( const std::string& folder )
{
	m_impl->Save( folder );
}

#endif // TRINITY_AL_CAPTURE_ENABLED
