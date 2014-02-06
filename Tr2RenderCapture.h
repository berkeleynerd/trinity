#pragma once
#ifndef Tr2RenderCapture_h_
#define Tr2RenderCapture_h_

#if TRINITY_AL_CAPTURE_ENABLED
class Tr2RenderCapture
{
public:
	Tr2RenderCapture();
	~Tr2RenderCapture();

	enum State
	{
		INACTIVE,		// caller should execute;
		RECORDING,		// being added to the list, caller should execute;
		PLAYING,		// playing from list, caller should immediately return;
		WAITING			// waiting for end of frame as our queue, caller should execute;
	};

	State	GetState() const { return m_state; }

	void	StartRecording();
	void	StopRecording();
	void	StartPlaying();
	void	StopPlaying();

	void	PresentHook();

	ALResult BeginScene() throw();
	ALResult EndScene() throw();
	
	ALResult SetStreamSource(		
		uint32_t stream, 
		const Tr2VertexBufferAL & buffer, 
		uint32_t offset, 
		uint32_t stride ) throw();

	ALResult SetIndices( const Tr2IndexBufferAL & buffer ) throw();
	ALResult SetTopology( Tr2RenderContextEnum::Topology topology ) throw();
	ALResult SetVertexLayout( Tr2VertexLayoutAL& layout ) throw();
	ALResult SetShader( const Tr2ShaderAL& shader ) throw();

	ALResult SetShaderBuffer( 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer ) throw();

	ALResult SetTexture( 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR ) throw();

	ALResult DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex = 0 ) throw();

	ALResult DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount ) throw();

	ALResult DrawIndexedInstanced(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t numInstances ) throw();

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride ) throw();

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride) throw();

	ALResult DrawPrimitiveUP( 
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t VertexStreamZeroStride ) throw();

	ALResult SetConstants( 
		const Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t unusedArgument = 0 ) throw();

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value ) throw();

	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count ) throw();

	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq ) throw();

	ALResult SetScissorRect( 
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom ) throw();

	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber ) throw();

	// Set a depth stencil.  Ideally you'd set renderTarget and depthStencil at the same time.
	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil ) throw();
	void SetReadOnlyDepth( bool enable ) throw();
	ALResult SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot = 0 ) throw();
	//Tr2RenderTargetAL GetRenderTarget( unsigned slot = 0 );


	// Helper function to clear the current primary backbuffer, depth and/or stencil.
	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0 ) throw();

	// rather hacky call to make integer loops work across DX9 and DX11.
	void	SetNumberOfLights(		uint32_t numLights ) throw();

	ALResult SetViewport( const Tr2Viewport& viewport ) throw();

	ALResult PushRenderTarget( uint32_t slot = 0 ) throw();
	ALResult PopRenderTarget( uint32_t slot = 0 ) throw();
	ALResult PushDepthStencil() throw();
	ALResult PopDepthStencil() throw();

	// Non-rendercontext calls that we need to know about
	ALResult Resolve( 
		const Tr2RenderTargetAL& destination, 
		const Tr2RenderTargetAL& source ) throw();

	void Save( const std::string& folder );

private:
	State	m_state;
	struct	Impl;
	std::unique_ptr<Impl>	m_impl;

	void	PlayFrame();

	Tr2RenderCapture( const Tr2RenderCapture & );
	Tr2RenderCapture& operator=( const Tr2RenderCapture & );
};

extern Tr2RenderCapture	s_renderCapture;

#define	TRINITY_AL_CAPTURE(func, ...)									\
	if( s_renderCapture.GetState() == Tr2RenderCapture::PLAYING )		\
	{																	\
		return S_OK;													\
	}																	\
	if( s_renderCapture.GetState() == Tr2RenderCapture::RECORDING )		\
	{																	\
		s_renderCapture. func( __VA_ARGS__ );							\
	}																	\

#define	TRINITY_AL_CAPTURE_NORETVAL(func, ...)							\
	if( s_renderCapture.GetState() == Tr2RenderCapture::PLAYING )		\
	{																	\
		return;															\
	}																	\
	if( s_renderCapture.GetState() == Tr2RenderCapture::RECORDING )		\
	{																	\
		s_renderCapture. func( __VA_ARGS__ );							\
	}																	\


#define	TRINITY_AL_CAPTURE_PRESENT										\
	s_renderCapture.PresentHook();										\
	
#else	 // ?TRINITY_AL_CAPTURE_ENABLED

#	define	TRINITY_AL_CAPTURE(func, ...)
#	define	TRINITY_AL_CAPTURE_NORETVAL(func, ...)
#	define	TRINITY_AL_CAPTURE_PRESENT

#endif // if TRINITY_AL_CAPTURE_ENABLED

#endif //Tr2RenderCapture_h_
