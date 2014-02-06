#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2RenderTargetALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2RenderTargetAL::Tr2RenderTargetAL()
	:m_msaaType( 1 ),
	m_msaaQuality( 0 ),
	m_expectedLabel( 0 ),
	m_actualLabel( nullptr ),
	m_colorMemory( nullptr ),
	m_cmaskMemory( nullptr ),
	m_fmaskMemory( nullptr ),
	m_frameUsed( 0 ),
	m_isAttached( false )
{
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

Tr2RenderTargetAL::~Tr2RenderTargetAL()
{
	Destroy();
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality,
	Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_RENDER_TARGET );
	CCP_STATS_ZONE( __FUNCTION__ );

	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	sce::Gnm::DataFormat dataFormat;
	if( !Tr2RenderContextAL::ConvertPixelFormatToDataFormat( format, dataFormat ) )
	{
		CCP_AL_LOGERR( "Tr2RenderTargetAL::Create: unsupported texture format %i", int( format ) );
		return E_INVALIDARG;
	}

	if( msaaType > 1 )
	{
		mipLevelCount = 1;

		sce::Gnm::NumSamples numSamples;
		sce::Gnm::NumFragments numFragments;
		if( msaaType >= 16 )
		{
			numSamples = sce::Gnm::kNumSamples16;
			numFragments = sce::Gnm::kNumFragments8;
		}
		else if( msaaType >= 8 )
		{
			numSamples = sce::Gnm::kNumSamples8;
			numFragments = sce::Gnm::kNumFragments8;
		}
		else if( msaaType >= 4 )
		{
			numSamples = sce::Gnm::kNumSamples4;
			numFragments = sce::Gnm::kNumFragments4;
		}
		else if( msaaType >= 2 )
		{
			numSamples = sce::Gnm::kNumSamples2;
			numFragments = sce::Gnm::kNumFragments2;
		}
		else
		{
			numSamples = sce::Gnm::kNumSamples1;
			numFragments = sce::Gnm::kNumFragments1;
		}

		sce::Gnm::TileMode tileMode;
		sce::GpuAddress::computeSurfaceTileMode(
			&tileMode, 
			sce::GpuAddress::kSurfaceTypeColorTargetDisplayable, 
			dataFormat, 
			std::max( 1u, std::min( msaaType, 16u ) ) );

		sce::Gnm::SizeAlign cmaskSizeAlign, fmaskSizeAlign;

		m_renderTargets.resize( 1 );
		auto sizeAlign = m_renderTargets[0].init( width, height, 1, dataFormat, tileMode, numSamples, numFragments, &cmaskSizeAlign, &fmaskSizeAlign );
		if( sizeAlign.m_size == 0 )
		{
			return E_FAIL;
		}
		void* memory = Tr2MemoryAllocator::AllocateGarlic( sizeAlign );
		if( !memory )
		{
			return E_OUTOFMEMORY;
		}
		void* cmaskMemory = Tr2MemoryAllocator::AllocateGarlic( cmaskSizeAlign );
		if( !cmaskMemory )
		{
			Tr2MemoryAllocator::Release( memory );
			return E_OUTOFMEMORY;
		}
		void* fmaskMemory = Tr2MemoryAllocator::AllocateGarlic( fmaskSizeAlign );
		if( !fmaskMemory )
		{
			Tr2MemoryAllocator::Release( memory );
			Tr2MemoryAllocator::Release( cmaskMemory );
			return E_OUTOFMEMORY;
		}
		m_renderTargets[0].setBaseAddress( memory );
		m_renderTargets[0].setCmaskAddress( cmaskMemory );
		m_renderTargets[0].setFmaskAddress( fmaskMemory );

		m_renderTargets[0].setFmaskCompressionEnable( true );
		m_renderTargets[0].setCmaskFastClearEnable( true );

		m_colorMemory = memory;
		m_cmaskMemory = cmaskMemory;
		m_fmaskMemory = fmaskMemory;
	}
	else
	{
		CR_RETURN_HR( m_backingStore.InternalCreateTexture2D( 
			width, 
			height, 
			mipLevelCount, 
			format, 
			sce::Gnm::kTileModeDisplay_2dThin, 
			sce::Gnm::kNumSamples1,
			Tr2BufferImplAL::READ_ONLY,
			Tr2BufferImplAL::READ_WRITE,
			renderContext ) );

		uint32_t trueMipLevelCount = m_backingStore.GetTrueMipCount();
		m_renderTargets.resize( trueMipLevelCount );

		sce::Gnm::Texture tex;
		m_backingStore.InternalGetTextureForGpu( COLOR_SPACE_LINEAR, tex, renderContext );
		for( uint32_t i = 0; i < trueMipLevelCount; ++i )
		{
			int32_t status = m_renderTargets[i].initFromTexture( &tex, i );
			if( status )
			{
				CCP_AL_LOGERR( "Tr2RenderTargetAL::Create: failed to create render target mip %u from backing store texture", unsigned( i ) );
				Destroy();
				return E_FAIL;
			}
		}
	}

	m_actualLabel = (volatile uint64_t*)Tr2MemoryAllocator::AllocateGarlic( sizeof( uint64_t ), 8 );
	*m_actualLabel = 0;
	m_expectedLabel = 0;

	m_mipCount = mipLevelCount;
	m_format = format;
	m_width = width;
	m_height = height;
	m_msaaType = msaaType;
	m_msaaQuality = msaaQuality;

	return S_OK;
}

ALResult Tr2RenderTargetAL::Attach( const sce::Gnm::RenderTarget& renderTarget, Tr2RenderContextEnum::PixelFormat format )
{
	Destroy();
	m_renderTargets.push_back( renderTarget );
	m_mipCount = 1;
	m_format = format;
	m_width = renderTarget.getWidth();
	m_height = renderTarget.getHeight();
	m_msaaType = 1 << renderTarget.getNumSamples();
	m_msaaQuality = 0;
	m_isAttached = true;
	m_actualLabel = (volatile uint64_t*)Tr2MemoryAllocator::AllocateGarlic( sizeof( uint64_t ), 8 );
	*m_actualLabel = 0;
	m_expectedLabel = 0;
	return S_OK;
}

bool Tr2RenderTargetAL::operator==( const Tr2RenderTargetAL& other ) const
{
	return this == &other;
}

bool Tr2RenderTargetAL::IsValid() const
{
	return !m_renderTargets.empty();
}

void Tr2RenderTargetAL::Destroy()
{
	m_backingStore.Destroy();
	m_renderTargets.clear();

	if( m_colorMemory )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_colorMemory );
		m_colorMemory = nullptr;
	}
	if( m_cmaskMemory )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_cmaskMemory );
		m_cmaskMemory = nullptr;
	}
	if( m_fmaskMemory )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_fmaskMemory );
		m_fmaskMemory = nullptr;
	}
	if( m_actualLabel )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, (void*)m_actualLabel );
		m_actualLabel = nullptr;
	}
	m_isAttached = false;
}

Tr2TextureAL& Tr2RenderTargetAL::GetTexture()
{
	return m_backingStore;
}

const Tr2TextureAL& Tr2RenderTargetAL::GetTexture() const
{
	return m_backingStore;
}

void Tr2RenderTargetAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
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
									d.m_mipCount, 
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

void Tr2RenderTargetAL::ReleaseALResource()
{
	if( !m_isAttached && !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_mipCount		= m_mipCount;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

ALResult Tr2RenderTargetAL::Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !destination.IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	return renderContext.InternalResolveRT( &destination, this );
}

ALResult Tr2RenderTargetAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
	if( m_mipCount == 1 || !IsValid() )
	{
		return S_OK;
	}
	return renderContext.InternalGenerateMips( m_backingStore );
}

ALResult Tr2RenderTargetAL::GetLockedRenderTarget( uint32_t mipLevel, uint32_t* ltrb, Tr2LockedRenderTargetAL& lockedRT, Tr2RenderContextAL& renderContext )
{
	sce::Gnmx::GfxContext& gfxc = renderContext.InternalGetGfxContext();
	renderContext.InternalSyncRenderTarget( *this );
	gfxc.submit();
	uint32_t wait = 0;
	while( m_expectedLabel != *m_actualLabel )
	{
		++wait;
	}

	void* data = nullptr;
	uint32_t pitch;
	CR_RETURN_HR( m_backingStore.Lock( mipLevel, ltrb, data, pitch, LOCK_READONLY, renderContext ) );
	lockedRT.m_lockedData.resize( "Tr2RenderTargetAL.m_lockedData", m_backingStore.GetMipSize( 0 ) );

	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture( &m_backingStore.m_texture, 0, 0 );
	sce::GpuAddress::detileSurface( lockedRT.m_lockedData.get(), data, &tp );

	lockedRT.m_pitch = m_backingStore.GetMipPitch( mipLevel );

	return S_OK;
}


ALResult Tr2RenderTargetAL::Lock(	
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL& renderContext )
{
	sce::Gnmx::GfxContext& gfxc = renderContext.InternalGetGfxContext();
	renderContext.InternalSyncRenderTarget( *this );
	gfxc.submit();
	uint32_t wait = 0;
	while( m_expectedLabel != *m_actualLabel )
	{
		++wait;
	}
	gfxc.reset();

	CR_RETURN_HR( m_backingStore.Lock( mipLevel, ltrb, data, pitch, LOCK_READONLY, renderContext ) );
	m_lockedData.resize( "Tr2RenderTargetAL.m_lockedData", m_backingStore.GetMipSize( 0 ) );

	sce::Gnm::Texture tex;
	m_backingStore.InternalGetTextureForGpu( COLOR_SPACE_LINEAR, tex, renderContext );
	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture( &m_backingStore.m_texture, 0, 0 );
	sce::GpuAddress::detileSurface( m_lockedData.get(), data, &tp );

	data = m_lockedData.get();
	pitch = m_backingStore.GetMipPitch( 0 );

	return S_OK;
}

ALResult Tr2RenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	m_lockedData.clear();
	return m_backingStore.Unlock( renderContext );
}

ALResult Tr2RenderTargetAL::CopySubresourceRegion( 
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !source.IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	return renderContext.InternalCopySubresourceRegion( *this, destX, destY, source, ltrb );
}

void Tr2RenderTargetAL::SetHintLockOften()
{
}

ALResult Tr2RenderTargetAL::InternalGetRenderTargetForGpu( uint32_t mipLevel, sce::Gnm::RenderTarget& rt, Tr2RenderContextAL& renderContext ) const
{
	if( !IsValid() || mipLevel >= GetTrueMipCount() )
	{
		return E_FAIL;
	}
	if( m_backingStore.IsValid() )
	{
		sce::Gnm::Texture tex;
		m_backingStore.InternalGetTextureForGpu( COLOR_SPACE_LINEAR, tex, renderContext );
		int32_t status = m_renderTargets[mipLevel].initFromTexture( &tex, mipLevel );
		if( status )
		{
			CCP_AL_LOGERR( "Tr2RenderTargetAL::InternalGetRenderTargetForGpu: failed to create render target mip %u from backing store texture", unsigned( mipLevel ) );
			return E_FAIL;
		}
		rt = m_renderTargets[mipLevel];
	}
	else
	{
		rt = m_renderTargets[mipLevel];
	}
	return S_OK;
}

void Tr2RenderTargetAL::InternalWriteSync( Tr2RenderContextAL& renderContext ) const
{
	sce::Gnmx::GfxContext& gfxc = renderContext.InternalGetGfxContext();
	m_expectedLabel++;
	gfxc.writeAtEndOfPipe( 
		sce::Gnm::kEopFlushCbDbCaches, 
		sce::Gnm::kEventWriteDestMemory, 
		(void *)m_actualLabel, 
		sce::Gnm::kEventWriteSource64BitsImmediate, 
		m_expectedLabel, 
		sce::Gnm::kCacheActionNone, 
		sce::Gnm::kCachePolicyLru );
}

#endif