#include "StdAfx.h"
#include "Tr2VertexBufferALDx11.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

using namespace Tr2RenderContextEnum;

// -------------------------------------------------------------
// Description:
//  Create a VB
// Arguments:
// length - number of BYTES to use in the vertex buffer
// -------------------------------------------------------------
ALResult Tr2VertexBufferAL::Create( uint32_t lengthInBytes,
									BufferUsage usage,
									const void* initialData,
									Tr2PrimaryRenderContextAL& renderContext )
{
	return CreateEx( lengthInBytes, usage, initialData, 0, renderContext );
}

ALResult Tr2VertexBufferAL::CreateEx( uint32_t lengthInBytes,
									  BufferUsage usage,
									  const void* initialData,
									  uint32_t/* exFlags */,
									  Tr2PrimaryRenderContextAL& renderContext )
{
	AL_FUZZ( OT_VERTEX_BUFFER );

	return Tr2BufferImplAL::Create( lengthInBytes,
									usage,
									D3D11_BIND_VERTEX_BUFFER,
									0,
									initialData,
									renderContext );
}

// -------------------------------------------------------------
// Description:
//  Create a VB with reasonable default flags.
// Arguments:
// length - number of BYTES to use in the vertex buffer
// -------------------------------------------------------------
ALResult Tr2VertexBufferAL::Create( uint32_t length,
									Tr2PrimaryRenderContextAL& renderContext )
{
	AL_FUZZ( OT_VERTEX_BUFFER );

	return Tr2BufferImplAL::Create( length,
									USAGE_CPU_READ | USAGE_CPU_WRITE,
									D3D11_BIND_VERTEX_BUFFER,
									0,
									nullptr,
									renderContext );
}

Tr2VertexBufferAL& Tr2VertexBufferAL::operator=( Tr2VertexBufferAL&& other )
{
	Tr2BufferImplAL::operator=( std::move( other ) );
	return *this;
}

#if TRINITY_AL_CAPTURE_ENABLED
ALResult Tr2VertexBufferAL::CloneTo( Tr2VertexBufferAL& target )
{
	void* data;
	auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();

	if( m_currentLock != LOCK_INVALID )
	{
		return E_FAIL;
	}

	CR_RETURN_HR( Lock( 0, 0, &data, LOCK_READONLY, renderContext ) );
#pragma warning( disable: 4189 )
	ON_BLOCK_EXIT( [&]{ Unlock( renderContext ); } );

	CR_RETURN_HR( target.Create( m_lengthInBytes, 0, data, renderContext ) );
	target.m_writeLockCount = m_writeLockCount;

	return S_OK;
}
#endif


#endif
