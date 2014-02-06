#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2ConstantBufferALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	:m_usage( 0 ),
	m_size( 0 ),
	m_frameUse( FRAME_USE_NOT_USED_YET )
{
}

Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
{
	Destroy();
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext )
{
	AL_FUZZ( OT_CONSTANT_BUFFER );
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	Destroy();

	if( ( usage & USAGE_LOCK_FREQUENTLY ) != 0 )
	{
		m_bufferMirror.resize( "Tr2ConstantBufferAL.m_bufferMirror", size );
		if( initialData )
		{
			memcpy( m_bufferMirror.get(), initialData, size );
		}

		m_size = size;
		m_frameUse = FRAME_USE_NOT_USED_YET;
		m_usage = usage;

		return S_OK;
	}

	void* initialDataPtr = nullptr;

	CR_RETURN_HR( m_buffer.Create( 
		size, 
		sce::Gnm::kAlignmentOfBufferInBytes,
		Tr2BufferImplAL::READ_WRITE, 
		Tr2BufferImplAL::READ_ONLY,
		Tr2BufferImplAL::DYNAMIC, 
		Tr2MemoryAllocator::ONION,
		&initialDataPtr, 
		renderContext ) );
	if( initialData )
	{
		memcpy( initialDataPtr, initialData, size );
	}
	m_size = size;
	m_frameUse = FRAME_USE_NOT_USED_YET;
	m_usage = usage;

	return S_OK;

}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL & renderContext )
{
	if( !IsValid() )
	{
		*data = 0;
		return E_FAIL;
	}

	CCP_ASSERT( m_frameUse != FRAME_USE_MIRRORED );	// if this fires, you've used GetBufferMirror, and should call UpdateFromMirror.
	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING  );	// if this fires, you've already locked this CB without then having bound it to the device.
													// Since CBs are always DISCARD, whatever you did in that previous Lock will be lost in this
													// one, so that's almost certainly not what you want.
	m_frameUse = FRAME_USE_LOCKING;

	bool isDynamic = ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0;

	if( isDynamic )
	{
		*data = m_bufferMirror.get();
		return S_OK;
	}
	else
	{
		*data = m_buffer.GetMemoryForCpuWriting( renderContext );
		return S_OK;
	}
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL & renderContext )
{
	AL_FUZZ_LOCK( OT_CONSTANT_BUFFER );

	CCP_ASSERT( m_frameUse != FRAME_USE_MIRRORED );	// if this fires, you've used GetBufferMirror, and should call UpdateFromMirror.
	CCP_ASSERT( m_frameUse == FRAME_USE_LOCKING	 );	// if this fires, you didn't call Lock (or it didn't work and you ignored HR).
	return S_OK;
}

bool Tr2ConstantBufferAL::IsValid() const
{
	bool isDynamic = ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0;

	if( isDynamic )
	{
		return m_bufferMirror;
	}
	else 
	{
		return m_buffer.IsValid();
	}
}

void Tr2ConstantBufferAL::Destroy()
{
	m_bufferMirror.clear();
	m_usage = 0;
	m_size = 0;
	m_buffer.Destroy();
	m_frameUse = FRAME_USE_NOT_USED_YET;
}

void* Tr2ConstantBufferAL::GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL & renderContext )
{
	AL_FUZZ_RET( OT_CONSTANT_BUFFER, nullptr );

	if( minimumSize > GetSize() )
	{
		if( FAILED( Create( minimumSize, m_usage, nullptr, Tr2RenderContextAL::GetPrimaryRenderContext() ) ) )
		{
			return nullptr;
		}
	}

	if( m_size == 0 )
	{
		return nullptr;
	}

	if( m_bufferMirror.size() < m_size )
	{
		m_bufferMirror.resize( "Tr2ConstantBufferAL::m_bufferMirror", m_size );
	}

	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING );	// if this fires, you've used Lock, and should call Unlock.
	m_frameUse = FRAME_USE_MIRRORED;

	return m_bufferMirror.get();
}

ALResult Tr2ConstantBufferAL::UpdateFromMirror( Tr2RenderContextAL & renderContext )
{
	AL_FUZZ( OT_CONSTANT_BUFFER );

	bool isDynamic = ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0;
	if( ( !isDynamic && !IsValid() ) || m_bufferMirror.size() < m_size || m_bufferMirror.empty() )
	{
		return E_FAIL;
	}

#if TRINITY_AL_CAPTURE_ENABLED
	++m_writeLockCount;
#endif

	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING );	// if this fires, you've used Lock, and should call Unlock.
	m_frameUse = FRAME_USE_MIRRORED;	// might not be set yet -- which is fine, the mirror persists across frames (but the CB itself might be shared)

	if( isDynamic )
	{
		return S_OK;
	}

	void* data = m_buffer.GetMemoryForCpuWriting( renderContext );
	memcpy( data, m_bufferMirror.get(), m_size );
    return S_OK;
}

#endif