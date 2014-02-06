#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2FenceALOrbis.h"

Tr2FenceAL::Tr2FenceAL()
	:m_isValid( false ),
	m_fence( nullptr )
{
}

Tr2FenceAL::~Tr2FenceAL()
{
	Destroy();
}

ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	m_isValid = true;
	return S_OK;
}

void Tr2FenceAL::Destroy()
{
	m_fence = nullptr;
	m_isValid = false;
}

bool Tr2FenceAL::IsValid() const
{
	return m_isValid;
}

ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}

	auto& gfxc = renderContext.InternalGetGfxContext();

	m_fence = static_cast<volatile uint64_t*>( gfxc.allocateFromCommandBuffer( sizeof( uint64_t ), sce::Gnm::kEmbeddedDataAlignment4 ) );
	if( !m_fence )
	{
		return E_FAIL;
	}
	*m_fence = 0;
	gfxc.writeAtEndOfPipe( 
		sce::Gnm::kEopFlushCbDbCaches, 
		sce::Gnm::kEventWriteDestMemory, 
		(void *)m_fence, 
		sce::Gnm::kEventWriteSource64BitsImmediate, 
		1, 
		sce::Gnm::kCacheActionNone, 
		sce::Gnm::kCachePolicyLru );
	return S_OK;
}

ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& )
{
	if( !m_fence )
	{
		return E_INVALIDCALL;
	}
	isReached = m_fence != 0;
	return S_OK;
}

ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& )
{
	return E_FAIL;
}


#endif