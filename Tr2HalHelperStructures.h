#pragma once
#ifndef Tr2HalHelperStructures_h_
#define Tr2HalHelperStructures_h_

#include "Tr2RenderContextEnum.h"

// -------------------------------------------------------------
// Description:
//	A simple structure to hold data used for initializing USAGE_IMMUTABLE textures
//	with intial pixel data.
// -------------------------------------------------------------
struct Tr2SubresourceData
{
	void*		m_sysMem;			// pointer to pixels for this mip
	uint32_t	m_sysMemPitch;		// size in bytes of one line of pixels
	uint32_t	m_sysMemSlicePitch;	// size in bytes of entire mip level. cannot be zero!
	uint32_t	m_height;			// number of lines in this mip level
};

// -------------------------------------------------------------
// Description:
//  Viewport for use with Tr2RenderContextAL.
// -------------------------------------------------------------
struct Tr2Viewport
{
	Tr2Viewport() {}

	Tr2Viewport( uint32_t width, uint32_t height )
	: m_x(0)
	, m_y(0)
	, m_width( (float)width )
	, m_height( (float)height )
	, m_minZ(0)
	, m_maxZ(1.0f)
	{}
	float	m_x;
	float	m_y;
	float	m_width;
	float	m_height;
	float	m_minZ;
	float	m_maxZ;
};

struct Tr2BitmapDimensions;

// --------------------------------------------------------------------------------------
// Description:
//   Descibes region of texture (range of cubemap faces, mip levels, rectangle/box 
//   coordinates) for Tr2TextureAL::CopySubresourceRegion member function. 
// --------------------------------------------------------------------------------------
struct Tr2TextureSubresource
{
	Tr2TextureSubresource();
	explicit Tr2TextureSubresource( Tr2RenderContextEnum::CubemapFace face );
	Tr2TextureSubresource( Tr2RenderContextEnum::CubemapFace face, uint32_t mipLevel );
	explicit Tr2TextureSubresource( uint32_t mipLevel );

	void ClampToTexture( const Tr2BitmapDimensions& texture );
	bool IsSubresourceFull( const Tr2BitmapDimensions& texture ) const;

	bool operator==( const Tr2TextureSubresource& other ) const;
	bool IsValid() const;

	uint32_t GetWidth()		const { return m_right - m_left; }
	uint32_t GetHeight()	const { return m_bottom - m_top; }
	uint32_t GetMipCount()	const { return m_endMipLevel - m_startMipLevel; }
	uint32_t GetDepth()		const { return m_back - m_front; }
	uint32_t GetFaceCount()	const { return m_endFace - m_startFace; }

	// Starting cubemap face
	Tr2RenderContextEnum::CubemapFace m_startFace;
	// One past ending cubemap face
	Tr2RenderContextEnum::CubemapFace m_endFace;
	// Starting mip level (0-based)
	uint32_t m_startMipLevel;
	// One past ending mip level
	uint32_t m_endMipLevel;
	uint32_t m_left;
	uint32_t m_top;
	uint32_t m_front;
	// Ignored for destination
	uint32_t m_right;
	// Ignored for destination
	uint32_t m_bottom;
	// Ignored for destination
	uint32_t m_back;
};

// --------------------------------------------------------------------------------------
// Description:
//   Descibes texture sampler. Used in creation of Tr2SamplerStateAL objects.  
// --------------------------------------------------------------------------------------
struct Tr2SamplerDescription
{
	Tr2SamplerDescription( 
		Tr2RenderContextEnum::TextureFilter minFilter,
		Tr2RenderContextEnum::TextureFilter magFilter,
		Tr2RenderContextEnum::TextureFilter mipFilter,
		bool isComparisonFilter,
		Tr2RenderContextEnum::TextureAddressMode addressU,
		Tr2RenderContextEnum::TextureAddressMode addressV,
		Tr2RenderContextEnum::TextureAddressMode addressW,
		float mipLODBias,
		uint32_t maxAnisotropy,
		Tr2RenderContextEnum::CompareFunc comparisonFunc,
		const float* borderColor,
		float minLOD,
		float maxLOD,
		bool isSRGBTexture )
	:	m_minFilter( minFilter ),
		m_magFilter( magFilter ),
		m_mipFilter( mipFilter ),
		m_isComparisonFilter( isComparisonFilter ),
		m_addressU( addressU ),
		m_addressV( addressV ),
		m_addressW( addressW ),
		m_mipLODBias( mipLODBias ),
		m_maxAnisotropy( maxAnisotropy ),
		m_comparisonFunc( comparisonFunc ),		
		m_minLOD( minLOD ),
		m_maxLOD( maxLOD ),
		m_isSRGBTexture( isSRGBTexture )
	{
		m_borderColor[0] = borderColor[0];
		m_borderColor[1] = borderColor[1];
		m_borderColor[2] = borderColor[2];
		m_borderColor[3] = borderColor[3];
	}

	bool operator==( const Tr2SamplerDescription& other ) const
	{
		return m_minFilter == other.m_minFilter &&
			m_magFilter == other.m_magFilter &&
			m_mipFilter == other.m_mipFilter &&
			m_isComparisonFilter == other.m_isComparisonFilter &&
			m_addressU == other.m_addressU &&
			m_addressV == other.m_addressV &&
			m_addressW == other.m_addressW &&
			m_mipLODBias == other.m_mipLODBias &&
			m_maxAnisotropy == other.m_maxAnisotropy &&
			m_comparisonFunc == other.m_comparisonFunc &&
			m_borderColor[0] == other.m_borderColor[0] &&
			m_borderColor[1] == other.m_borderColor[1] &&
			m_borderColor[2] == other.m_borderColor[2] &&
			m_borderColor[3] == other.m_borderColor[3] &&
			m_minLOD == other.m_minLOD &&
			m_maxLOD == other.m_maxLOD &&
			m_isSRGBTexture == other.m_isSRGBTexture;
	}

	Tr2RenderContextEnum::TextureFilter m_minFilter;
	Tr2RenderContextEnum::TextureFilter m_magFilter;
	Tr2RenderContextEnum::TextureFilter m_mipFilter;
	bool m_isComparisonFilter;
	Tr2RenderContextEnum::TextureAddressMode m_addressU;
	Tr2RenderContextEnum::TextureAddressMode m_addressV;
	Tr2RenderContextEnum::TextureAddressMode m_addressW;
	float m_mipLODBias;
	uint32_t m_maxAnisotropy;
	Tr2RenderContextEnum::CompareFunc m_comparisonFunc;
	float m_borderColor[4];
	float m_minLOD;
	float m_maxLOD;
	bool m_isSRGBTexture;
};

// -------------------------------------------------------------
// Description:
// This is a null depth stencil. If you want to disable depth testing, push or set this one.
// -------------------------------------------------------------
extern const Tr2DepthStencilAL	nullDS;

// -------------------------------------------------------------
// Description:
// These are null shaders. If you want to disable a shader stage, set any of these.
// -------------------------------------------------------------
extern const Tr2ShaderAL		nullShader[Tr2RenderContextEnum::SHADER_TYPE_COUNT];

// -------------------------------------------------------------
// Description:
// This is a null index buffer. If you want to unbind any currently bound index buffers, set this one.
// -------------------------------------------------------------
extern const Tr2IndexBufferAL	nullIB;

// -------------------------------------------------------------
// Description:
// This is a null constant buffer. If you want to unbind any currently bound constant buffers, set this one.
// -------------------------------------------------------------
extern const Tr2ConstantBufferAL	nullCB;

// -------------------------------------------------------------
// Description:
// This is a null vertex buffer. If you want to unbind any currently bound vertex buffers, set this one.
// -------------------------------------------------------------
extern const Tr2VertexBufferAL	nullVB;

// -------------------------------------------------------------
// Description:
// This is a null texture. If you want to unbind any currently bound texture, set this one.
// -------------------------------------------------------------
extern const Tr2TextureAL		nullTX;

// -------------------------------------------------------------
// Description:
// This is a null renderTarget. When bound to slot 0, this will bind the swapChain's main backbuffer,
// since not having a renderTarget bound is not allowed.
// For slots 1 and up, this will unbind any currently bound renderTarget and therefor disable PS out for
// that slot.
// -------------------------------------------------------------
extern const Tr2RenderTargetAL	nullRT;

// -------------------------------------------------------------
// Description:
// This is a null UAV buffer. If you want to unbind any currently bound buffer, set this one.
// -------------------------------------------------------------
extern const Tr2GpuBufferAL		nullGB;

// -------------------------------------------------------------
// Description:
// Helper to keep track of variables that usually go together, 
// and functions to do some repeated math around them (computing
// size of a given miplevel, etc).
// -------------------------------------------------------------
struct Tr2BitmapDimensions
{	
	Tr2BitmapDimensions( uint32_t							width = 0,
						 uint32_t							height = 0,
						 uint32_t							mipCount = 0,
						 Tr2RenderContextEnum::PixelFormat	format = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
	: m_width( width )
	, m_height( height )
	, m_volumeDepth( 0 )
	, m_mipCount( mipCount )
	, m_type( Tr2RenderContextEnum::TEX_TYPE_INVALID )
	, m_format( format )
	{}

	uint32_t							GetWidth()  const		{ return m_width; }
	uint32_t							GetHeight() const		{ return m_height; }
	uint32_t							GetDepth() const		{ return m_volumeDepth; }
	Tr2RenderContextEnum::PixelFormat	GetFormat() const		{ return m_format; }
	uint32_t							GetMipCount() const		{ return m_mipCount; }
	uint32_t							GetTrueMipCount() const;
	bool								IsCompressed() const	{ return IsCompressedFormat( m_format ); }
	uint32_t							HasMipmap()	const		{ return m_mipCount != 1; }

	uint32_t							GetMipWidth	 ( uint32_t level ) const;
	uint32_t							GetMipHeight ( uint32_t level ) const;
	uint32_t							GetMipDepth  ( uint32_t level ) const;
	uint32_t							GetMipPitch  ( uint32_t level ) const;	
	uint32_t							GetMipSize	 ( uint32_t level ) const;
	// Number of rows to copy in a mip. For non-compressed formats this is the same as GetMipHeight.
	// For compressed formats it's that number / 4.
	uint32_t							GetMipNumRows( uint32_t level ) const
	{ return IsCompressed() ? GetMipHeight( level ) / 4 : GetMipHeight( level ); }

	Tr2RenderContextEnum::TextureType	GetType()		const { return m_type; }

protected:
	uint32_t							m_width;
	uint32_t							m_height;
	uint32_t							m_volumeDepth;
	uint32_t							m_mipCount;
	Tr2RenderContextEnum::TextureType	m_type;
	Tr2RenderContextEnum::PixelFormat	m_format;

	void Destroy()
	{
		// m_width	= m_height = m_volumeDepth = m_mipCount = 0;
		m_type	 = Tr2RenderContextEnum::TEX_TYPE_INVALID;
		m_format = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	}
};

inline uint32_t Tr2BitmapDimensions::GetMipWidth( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_width >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_width >> level, 1u );
}

inline uint32_t Tr2BitmapDimensions::GetMipHeight( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_height >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_height >> level, 1u );
}

inline uint32_t Tr2BitmapDimensions::GetMipDepth( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_volumeDepth >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_volumeDepth >> level, 1u );
}
/*
inline uint32_t Tr2BitmapDimensions::GetTrueMipCount() const
{
	return	m_mipCount >= 1		? m_mipCount 
								: uint32_t( 0.5 + log( std::max<double>( m_height, m_width ) ) / log(2.0) ) + 1;
}
*/
inline uint32_t Tr2BitmapDimensions::GetMipPitch( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return GetMipWidth( level ) / 4u * GetBlockByteSize( m_format );
	}

	return GetMipWidth( level ) * GetBytesPerPixel( m_format );
}

inline uint32_t Tr2BitmapDimensions::GetMipSize( uint32_t level ) const
{
	if( IsCompressed() )
	{
		return GetMipWidth( level ) * GetMipHeight( level ) / 16 * GetBlockByteSize( m_format );
	}
	
	return GetMipWidth( level ) * GetMipHeight( level ) * GetBytesPerPixel( m_format );
}

bool Crop(	Tr2TextureSubresource& sourceSR,
			const Tr2BitmapDimensions& sourceBD, 
			Tr2TextureSubresource& destSR,
			const Tr2BitmapDimensions& destBD );

void AdvanceMip( Tr2TextureSubresource& sub, const Tr2BitmapDimensions& bd, uint32_t mip );

#endif //Tr2HalHelperStructures_h_
