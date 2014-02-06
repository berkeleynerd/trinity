#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2VertexLayoutALOrbis.h"
#include "Tr2MemoryAllocator.h"

using namespace Tr2RenderContextEnum;

Tr2VertexLayoutAL::Tr2VertexLayoutAL()
	:m_isValid( false ),
	m_frameUsed( 0 )
{
}

Tr2VertexLayoutAL::~Tr2VertexLayoutAL()
{
	Destroy();
}

#if TRINITY_HAVE_CPP0X
Tr2VertexLayoutAL::Tr2VertexLayoutAL( Tr2VertexLayoutAL&& other )
	:m_definition( std::move( other.m_definition ) )
{
}

Tr2VertexLayoutAL& Tr2VertexLayoutAL::operator=( Tr2VertexLayoutAL&& other )
{
	if( this == &other )
	{
		return *this;
	}

	m_definition.release();
	m_definition = std::move( other.m_definition );

	return *this;
}
#endif // TRINITY_HAVE_CPP0X

ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_VERTEX_LAYOUT );
	CCP_STATS_ZONE( __FUNCTION__ );
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	m_definition = definition;
	if( definition.m_items.empty() )
	{
		return E_FAIL;
	}
	m_isValid = true;
	m_frameUsed = renderContext.InternalGetCurentFrameIndex() + renderContext.InternalGetMaxFrameLatency() + 1;

	return S_OK;
}

ALResult Tr2VertexLayoutAL::SetLayout( const Tr2ShaderAL* /*vertexShader*/, Tr2RenderContextAL& /*renderContext*/ )
{
	// We set the actual layout in Tr2RenderContextAL::ApplyShadowRenderStates
	return E_FAIL;
}

bool Tr2VertexLayoutAL::IsValid() const
{
	return m_isValid;
}

void Tr2VertexLayoutAL::Destroy()
{
	m_definition.m_items.clear();
	m_isValid = false;
	for( auto it = m_fetchShaders.begin(); it != m_fetchShaders.end(); ++it )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, it->second.shader );
	}
	m_fetchShaders.clear();
}

#endif