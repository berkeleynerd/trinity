#include "StdAfx.h"
#include "Tr2DepthStencil.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencil::Tr2DepthStencil( IRoot* )
{	
}

Tr2DepthStencil::~Tr2DepthStencil()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void Tr2DepthStencil::py__init__(
		Be::OptionalWithDefaultValue<unsigned, 0> width,
		Be::OptionalWithDefaultValue<unsigned, 0> height,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::DepthStencilFormat, Tr2RenderContextEnum::DSFMT_UNKNOWN> format,
		Be::OptionalWithDefaultValue<unsigned, 1> msaaType,
		Be::OptionalWithDefaultValue<unsigned, 0> msaaQuality,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::ExFlag, Tr2RenderContextEnum::EX_NONE> flags )
{
	if( width && height && format )
	{
		Create( width, height, format, msaaType, msaaQuality, flags );		
	}		
}

long Tr2DepthStencil::Create( unsigned width, unsigned height, DepthStencilFormat dsFormat, unsigned msaaType, unsigned msaaQuality, Tr2RenderContextEnum::ExFlag flags )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_depthStencil.Create( width, height, dsFormat, Tr2MsaaDesc( msaaType, msaaQuality ), flags, renderContext ).GetResult();
}

Tr2TextureAL* Tr2DepthStencil::GetTexture()
{
	if( m_depthStencil.IsValid() && m_depthStencil.GetTexture().IsValid() )
	{
		return  &m_depthStencil.GetTexture();
	}
	return nullptr;
}

bool Tr2DepthStencil::IsReadable() const
{
	return m_depthStencil.GetTexture().IsValid() && m_depthStencil.GetMsaaDesc().samples < 2;
}

bool Tr2DepthStencil::IsValid() const
{
	CCP_STATS_ZONE( __FUNCTION__ );
	return m_depthStencil.IsValid();
}	

void Tr2DepthStencil::Destroy()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	m_depthStencil.Destroy();
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the object contains a reference to given AL object. This method is exposed
//   to Python and is used for debugging.
// Arguments:
//   type - Tr2RenderContextEnum::ObjectType, type of AL object
//   object - pointer to an AL object (passed as a number)
// Return Value:
//   true If object contains a reference to the given AL object
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DepthStencil::HasALObject( int type, size_t object )
{
	if( type == OT_DEPTH_STENCIL )
	{
		return m_depthStencil == *reinterpret_cast<Tr2DepthStencilAL*>( object );
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns DirectX shared handle of contained AL depth stencil.
// Return Value:
//   Shared handle of contained AL depth stencil
// --------------------------------------------------------------------------------------
uintptr_t Tr2DepthStencil::GetSharedHandle() const
{
	return m_depthStencil.GetSharedHandle();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetWidth() const
{
	return m_depthStencil.GetWidth();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetHeight() const
{
	return m_depthStencil.GetHeight();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetMsaaSamples() const
{
	return m_depthStencil.GetMsaaDesc().samples;
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetMsaaQuality() const
{
	return m_depthStencil.GetMsaaDesc().quality;
}

// --------------------------------------------------------------------------------------
Tr2RenderContextEnum::DepthStencilFormat Tr2DepthStencil::GetFormat() const
{
	return m_depthStencil.GetFormat();
}

