#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2DepthStencilALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "Tr2BufferImplALOrbis.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	:m_width( 0 ),
	m_height( 0 ),
	m_format( DSFMT_UNKNOWN ),
	m_msaaType( 0 ),
	m_msaaQuality( 0 )
{
	m_depthStencil.setHtileAddress( nullptr );
	m_depthStencil.setAddresses( nullptr, nullptr );
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
	Destroy();
}

ALResult Tr2DepthStencilAL::CreateEx(	
	uint32_t width, 
	uint32_t height, 
	Tr2RenderContextEnum::DepthStencilFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	uint32_t /*flags*/, 
	Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( width, height, format, msaaType, msaaQuality, renderContext );
}

ALResult Tr2DepthStencilAL::Create(
	uint32_t width, 
	uint32_t height, 
	Tr2RenderContextEnum::DepthStencilFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	Tr2PrimaryRenderContextAL& renderContext )
{
	AL_FUZZ( OT_DEPTH_STENCIL );
	CCP_STATS_ZONE( __FUNCTION__ );

	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	sce::Gnm::ZFormat zFormat;
	switch( format )
	{
	case DSFMT_D16_LOCKABLE:
	case DSFMT_D15S1:
	case DSFMT_D16:
		zFormat = sce::Gnm::kZFormat16;
		break;
	default:
		zFormat = sce::Gnm::kZFormat32Float;
		break;
	}
	sce::Gnm::StencilFormat stencilFormat;
	switch( format )
	{
	case DSFMT_D24S8:
	case DSFMT_D24FS8:
	case DSFMT_READABLE:
	case DSFMT_D15S1:
	case DSFMT_D24X4S4:
		stencilFormat = sce::Gnm::kStencil8;
		break;
	default:
		stencilFormat = sce::Gnm::kStencilInvalid;
		break;
	}

	sce::Gnm::DataFormat depthFormat = sce::Gnm::DataFormat::build( zFormat );
	sce::Gnm::TileMode depthTileMode;
	sce::GpuAddress::computeSurfaceTileMode( 
		&depthTileMode, 
		sce::GpuAddress::kSurfaceTypeDepthOnlyTarget, 
		depthFormat, 
		std::max( 1u, std::min( msaaType, 16u ) ) );

	sce::Gnm::SizeAlign htileSizeAlign;
	sce::Gnm::SizeAlign stencilSizeAlign;

	auto sizeAlign = m_depthStencil.init( width, height, depthFormat.getZFormat(), stencilFormat, depthTileMode, sce::Gnm::kNumFragments1, &stencilSizeAlign, nullptr /*&htileSizeAlign*/ );

	if( sizeAlign.m_size == 0 )
	{
		return E_FAIL;
	}

	m_zMemory = std::shared_ptr<Tr2BufferImplAL>( CCP_NEW( "Tr2DepthStencilAL::m_zMemory" ) Tr2BufferImplAL );
	void* memory = nullptr;
	CR_RETURN_HR( m_zMemory->Create( 
		sizeAlign.m_size,
		sizeAlign.m_align,
		Tr2BufferImplAL::READ_ONLY,
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::STATIC,
		Tr2MemoryAllocator::GARLIC,
		&memory,
		renderContext ) );

	void* stencilMemory = nullptr;
	if( stencilSizeAlign.m_size )
	{
		CR_RETURN_HR( m_stencilMemory.Create( 
			stencilSizeAlign.m_size,
			stencilSizeAlign.m_align,
			Tr2BufferImplAL::READ_ONLY,
			Tr2BufferImplAL::READ_WRITE,
			Tr2BufferImplAL::STATIC,
			Tr2MemoryAllocator::GARLIC,
			&stencilMemory,
			renderContext ) );
	}

	m_depthStencil.setHtileAddress( nullptr );
	m_depthStencil.setAddresses( memory, stencilMemory );

	if( format == DSFMT_READABLE )
	{
		CR_RETURN_HR( m_backingStore.InternalAttachToDepthBuffer( 
			m_depthStencil,
			m_zMemory,
			renderContext ) );
	}

	m_width = width;
	m_height = height;
	m_format = format;
	m_msaaType = msaaType;
	m_msaaQuality = msaaQuality;

	return S_OK;
}

void Tr2DepthStencilAL::Destroy()
{
	m_stencilMemory.Destroy();
	m_depthStencil.setAddresses( nullptr, nullptr );
	m_depthStencil.setHtileAddress( nullptr );
	m_backingStore.Destroy();
	m_width = 0;
	m_height = 0;
	m_format = DSFMT_UNKNOWN;
	m_msaaType = 0;
	m_msaaQuality = 0;	
}

void Tr2DepthStencilAL::ReleaseALResource()
{
	if( !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

void Tr2DepthStencilAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
	if( m_deviceLost.m_valid )
	{
		if( IsValid() )
		{
			m_deviceLost.m_valid = false;
		}
		else
		{
			const auto &d = m_deviceLost;

			if( d.m_width  > 0 && 
				d.m_height > 0 &&
				SUCCEEDED( Create(	d.m_width, 
									d.m_height, 
									d.m_format, 
									d.m_msaaType, 
									d.m_msaaQuality, 
									renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

bool Tr2DepthStencilAL::IsValid() const
{
	return m_depthStencil.getZReadAddress() != nullptr;
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	return m_backingStore;
}

const Tr2TextureAL& Tr2DepthStencilAL::GetTexture() const
{
	return m_backingStore;
}

bool Tr2DepthStencilAL::IsReadable() const
{
	return IsValid() && m_format == DSFMT_READABLE;
}

bool Tr2DepthStencilAL::operator==( const Tr2DepthStencilAL& other ) const
{
	return this == &other;
}

#endif