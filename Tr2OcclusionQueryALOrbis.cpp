#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2OcclusionQueryALOrbis.h"
#include "Tr2MemoryAllocator.h"

Tr2OcclusionQueryAL::Tr2OcclusionQueryAL()
	:m_occlusionResults( nullptr ),
	m_frameUsed( 0 )
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
	Destroy();
}

ALResult Tr2OcclusionQueryAL::Create( Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	Destroy();
	m_occlusionResults = reinterpret_cast<sce::Gnm::OcclusionQueryResults*>( 
		Tr2MemoryAllocator::AllocateGarlic( sizeof( sce::Gnm::OcclusionQueryResults ), 8 ) );
	if( !m_occlusionResults )
	{
		return E_OUTOFMEMORY;
	}
	m_occlusionResults->reset();
	m_frameUsed = renderContext.InternalGetCurentFrameIndex() + renderContext.InternalGetMaxFrameLatency() + 1;
	return S_OK;
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_occlusionResults;
}

void Tr2OcclusionQueryAL::Destroy()
{
	Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_occlusionResults );
	m_occlusionResults = nullptr;
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	auto& gfxc = renderContext.InternalGetGfxContext();
	m_occlusionResults->reset();
	gfxc.writeOcclusionQuery( sce::Gnm::kOcclusionQueryOpBegin, m_occlusionResults );
	gfxc.setDbCountControl( sce::Gnm::kDbCountControlZPassIncrementEnable, sce::Gnm::kDbCountControlPerfectZPassCountsEnable, 0 ); // TODO: should we turn this off? set MSAA?
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	auto& gfxc = renderContext.InternalGetGfxContext();
	gfxc.writeOcclusionQuery( sce::Gnm::kOcclusionQueryOpEnd, m_occlusionResults );
	gfxc.setDbCountControl( sce::Gnm::kDbCountControlZPassIncrementDisable, sce::Gnm::kDbCountControlPerfectZPassCountsEnable, 0 ); // TODO: should we turn this off? set MSAA?
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	if( m_occlusionResults->isReady() )
	{
		count = uint32_t( m_occlusionResults->getZPassCount() );
		return S_OK;
	}
	if( waitMode == WAIT )
	{
		// TODO: implement some kind of waiting here
		return E_FAIL;
	}
	return S_FALSE;
}

bool Tr2OcclusionQueryAL::operator==( const Tr2OcclusionQueryAL& other ) const
{
	return this == &other;
}

#endif