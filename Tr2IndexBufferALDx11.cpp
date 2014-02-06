#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2IndexBufferALDx11.h"

using namespace Tr2RenderContextEnum;

Tr2IndexBufferAL::Tr2IndexBufferAL()
	: m_numIndices( 0 )
	, m_is16Bit( false )
{}

// -------------------------------------------------------------
// Description:
//	Create an indexbuffer. NOTE that you pass the number of indices that you want, NOT the size in bytes
//	of the buffer.  That can be calculated from bitCount (which is an enum), so it would be redundant and error-prone
//	to ask a bytecount.
// Arguments:
// - numberOfIndices - number of indices, NOT bytes, to be stored in this index buffer.
// -------------------------------------------------------------
ALResult Tr2IndexBufferAL::Create(	
	uint32_t numberOfIndices, 
	Tr2RenderContextEnum::BufferUsage usage, 
	Tr2RenderContextEnum::IndexBufferBitcount bitCount, 
	const void* initialData, 
	Tr2PrimaryRenderContextAL &renderContext )
{
	AL_FUZZ( OT_INDEX_BUFFER );

	m_numIndices = numberOfIndices;
	m_is16Bit = bitCount == Tr2RenderContextEnum::IB_16BIT;
	return	Tr2BufferImplAL::Create( 
					GetTotalSizeInBytes(), 
					usage, 
					D3D11_BIND_INDEX_BUFFER, 
					0,
					initialData, 
					renderContext );
}

Tr2IndexBufferAL& Tr2IndexBufferAL::operator=( Tr2IndexBufferAL&& other )
{
	m_numIndices = other.m_numIndices;
	m_is16Bit    = other.m_is16Bit;
	Tr2BufferImplAL::operator=( std::move( other ) );
	return *this;
}

// -------------------------------------------------------------
// Description:
//  Check that the index buffer is a 16 bit index buffer, and if so try to lock and put the pointer in data.
// -------------------------------------------------------------
ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint16_t *&data, 
	Tr2RenderContextEnum::LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );

	if( !m_is16Bit )
	{
		data = nullptr;
		return E_INVALIDARG;
	}
	return Tr2BufferImplAL::Lock( offset, sizeInBytes, (void**)&data, lockType, renderContext );
}

// -------------------------------------------------------------
// Description:
//	Check that the index buffer is a 32 bit index buffer, and if so try to lock and put the pointer in data.
// -------------------------------------------------------------
ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint32_t *&data, 
	Tr2RenderContextEnum::LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );

	if( m_is16Bit )
	{
		data = nullptr;
		return E_INVALIDARG;
	}
	return Tr2BufferImplAL::Lock( offset, sizeInBytes, (void**)&data, lockType, renderContext );
}


#if TRINITY_AL_CAPTURE_ENABLED
ALResult Tr2IndexBufferAL::CloneTo( Tr2IndexBufferAL& target )
{
	void* data;

	if( m_currentLock != LOCK_INVALID )
	{
		return E_FAIL;
	}

	CR_RETURN_HR( Lock( 0, 0, &data, LOCK_READONLY, Tr2RenderContextAL::GetPrimaryRenderContext() ) );
#pragma warning( disable: 4189 )
	ON_BLOCK_EXIT( [&]{ Unlock( Tr2RenderContextAL::GetPrimaryRenderContext() ); } );

	CR_RETURN_HR( target.Create(	GetNumIndices(), 
									0,
									Is16Bit() ? IB_16BIT : IB_32BIT,
									data,
									Tr2RenderContextAL::GetPrimaryRenderContext() ) );

	target.m_writeLockCount = m_writeLockCount;

	return S_OK;
}
#endif

#endif