////////////////////////////////////////////////////////////
//
//    Created:   June 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2MouseCursor.h"
#include "Tr2HostBitmap.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MouseCursor default constructor
// --------------------------------------------------------------------------------------
Tr2MouseCursor::Tr2MouseCursor( IRoot* lockobj )
#ifdef _WIN32
:    m_cursor( nullptr )
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	, m_hotspotX( 0 )
	, m_hotspotY( 0 )
#endif
#elif __APPLE__
:   m_cursor( 0 )
#endif
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MouseCursor destructor
// --------------------------------------------------------------------------------------
Tr2MouseCursor::~Tr2MouseCursor()
{
#ifdef _WIN32
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
	if( m_cursor )
	{
		DeleteObject( m_cursor );
	}
#endif
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void Tr2MouseCursor::py__init__( Tr2HostBitmap* bitmap, unsigned hotspotX, unsigned hotspotY )
{
	if( bitmap )
	{
		Create( bitmap, hotspotX, hotspotY );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if the cursor is valid, i.e. was successfully created.
// Return Value:
//   true If the cursor has image
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2MouseCursor::IsValid() const
{
#ifdef _WIN32
	return m_cursor != nullptr;
#elif __APPLE__
    return m_cursor != 0;
#else
    return false;
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to convert color/alpha values in B5G6R5 + A8 format to B8G8R8A8 
//   format.
// Arguments:
//   color - Color value in B5G6R5 format.
//   alpha - 8 bit alpha value
// Return Value:
//   Color/alpha value in B8G8R8A8 format
// --------------------------------------------------------------------------------------
inline unsigned ConvertBGR565A8ToBGRA8( unsigned color, unsigned alpha ) 
{
	return unsigned( color & 0x1f ) * 255 / 31 | 
		( unsigned( ( color >> 5 ) & 0x3f ) * 255 / 63 ) << 8 | 
		( unsigned( ( color >> 11 ) & 0x1f ) * 255 / 31 ) << 16 | 
		( alpha << 24 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to decompress BC1 format image into B8G8R8A8 format. Used by mouse
//   cursors, which are small and are cached, so the function doesn't have to be super-
//   optimal. Also flips image vertically for DX11 and GL since DX and WINAPI use 
//   different origin.
// Arguments:
//   dest - Destination buffer
//   source - Source buffer
//   width - Image width
//   height - Image height
//   pitch - Destination image pitch
// --------------------------------------------------------------------------------------
static void DecompressBC1( char* dest, const char* source, unsigned width, unsigned height, unsigned pitch )
{
	for( unsigned j = 0; j < height; j += 4 )
	{
		for( unsigned i = 0; i < width; i += 4 )
		{
			unsigned color0 = *reinterpret_cast<const unsigned short*>( source );
			unsigned color1 = *reinterpret_cast<const unsigned short*>( source + 2 );
			unsigned bits = *reinterpret_cast<const unsigned*>( source + 4 );
			if( color0 > color1 )
			{
				for( unsigned y = 0; y < 4; ++y )
				{
					for( unsigned x = 0; x < 4; ++x )
					{
						unsigned destY = j + y;
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
						destY = height - 1 - destY;
#endif
						uint32_t* destPixel = reinterpret_cast<uint32_t*>( dest + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
						switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
						{
						case 0:
							*destPixel = ConvertBGR565A8ToBGRA8( color0, 255 );
							break;
						case 1:
							*destPixel = ConvertBGR565A8ToBGRA8( color1, 255 );
							break;
						case 2:
							*destPixel = ConvertBGR565A8ToBGRA8( (2*color0+color1)/3, 255 );
							break;
						case 3:
							*destPixel = ConvertBGR565A8ToBGRA8( (color0+2*color1)/3, 255 );
							break;
						}
					}
				}
			}
			else
			{
				for( unsigned y = 0; y < 4; ++y )
				{
					for( unsigned x = 0; x < 4; ++x )
					{
						unsigned destY = j + y;
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
						destY = height - 1 - destY;
#endif
						uint32_t* destPixel = reinterpret_cast<uint32_t*>( dest + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
						switch( ( bits >> 2*(4*y+x) ) & 3 )
						{
						case 0:
							*destPixel = ConvertBGR565A8ToBGRA8( color0, 255 );
							break;
						case 1:
							*destPixel = ConvertBGR565A8ToBGRA8( color1, 255 );
							break;
						case 2:
							*destPixel = ConvertBGR565A8ToBGRA8( (color0+color1)/2, 255 );
							break;
						case 3:
							*destPixel = ConvertBGR565A8ToBGRA8( (color0+2*color1)/3, 0 );
							break;
						}
					}
				}
			}
			source += 8;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to decompress BC2 format image into B8G8R8A8 format. Used by mouse
//   cursors, which are small and are cached, so the function doesn't have to be super-
//   optimal. Also flips image vertically for DX11 and GL since DX and WINAPI use 
//   different origin.
// Arguments:
//   dest - Destination buffer
//   source - Source buffer
//   width - Image width
//   height - Image height
//   pitch - Destination image pitch
// --------------------------------------------------------------------------------------
static void DecompressBC2( char* dest, const char* source, unsigned width, unsigned height, unsigned pitch )
{
	for( unsigned j = 0; j < height; j += 4 )
	{
		for( unsigned i = 0; i < width; i += 4 )
		{
			unsigned color0 = *reinterpret_cast<const unsigned short*>( source + 8 );
			unsigned color1 = *reinterpret_cast<const unsigned short*>( source + 8 + 2 );
			unsigned bits = *reinterpret_cast<const unsigned*>( source + 8 + 4 );
			for( unsigned y = 0; y < 4; ++y )
			{
				for( unsigned x = 0; x < 4; ++x )
				{
					unsigned destY = j + y;
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
					destY = height - 1 - destY;
#endif
					uint32_t* destPixel = reinterpret_cast<uint32_t*>( dest + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
					unsigned alphaValue = ( *( reinterpret_cast<const unsigned short*>( source ) + y ) >> x * 4 ) & 15;
					alphaValue = alphaValue * 255 / 15;
					switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
					{
					case 0:
						*destPixel = ConvertBGR565A8ToBGRA8( color0, alphaValue );
						break;
					case 1:
						*destPixel = ConvertBGR565A8ToBGRA8( color1, alphaValue );
						break;
					case 2:
						*destPixel = ConvertBGR565A8ToBGRA8( (2*color0+color1)/3, alphaValue );
						break;
					case 3:
						*destPixel = ConvertBGR565A8ToBGRA8( (color0+2*color1)/3, alphaValue );
						break;
					}
				}
			}
			source += 16;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a new cursor.
// Arguments:
//   bitmap - Bitmap containing cursor image. Needs to have B8G8R8A8 format and usually
//            should be 32 by 32 pixels.
//   hotspotX - X coordinate of hotspot
//   hotspotY - Y coordinate of hotspot
// Return Value:
//   true If the cursor was successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2MouseCursor::Create( Tr2HostBitmap* bitmap, int hotspotX, int hotspotY )
{
	if( bitmap == nullptr )
	{
		CCP_LOGERR( "Tr2MouseCursor.Create: nullptr is passed for bitmap" );
		return false;
	}

#ifdef _WIN32
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
	if( m_cursor )
	{
		DeleteObject( m_cursor );
	}
#endif
	m_cursor = nullptr;
#endif
	// Check the format: we really need B8G8R8A8 for cursor,
	// but we also support BC1 and BC3 for compatibility with
	// existing cursor images
	switch( bitmap->GetFormat() )
	{
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB:

	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB:

	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		break;
	default:
		CCP_LOGERR( "Tr2MouseCursor.Create: unsupported bitmap format (only B8G8R8A8, BC1 and BC2 are supported)" );
		return false;
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !renderContext.m_d3dDevice9 )
	{
		CCP_LOGERR( "Tr2MouseCursor.Create: no DX9 device available" );
		return false;
	}

	CR_RETURN_VAL( 
		renderContext.m_d3dDevice9->CreateOffscreenPlainSurface( 
			bitmap->GetWidth(), 
			bitmap->GetHeight(),
			D3DFMT_A8R8G8B8, 
			D3DPOOL_SYSTEMMEM, 
			&m_cursor, 
			nullptr ),
		false );

	D3DLOCKED_RECT rect;
	if( FAILED( m_cursor->LockRect( &rect, nullptr, 0 ) ) )
	{
		m_cursor = nullptr;
		return false;
	}
	switch( bitmap->GetFormat() )
	{
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB:
		DecompressBC1( reinterpret_cast<char*>( rect.pBits ), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), rect.Pitch );
		break;

	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB:
		DecompressBC2( reinterpret_cast<char*>( rect.pBits ), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), rect.Pitch );
		break;
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		{
			const char* row = bitmap->GetRawData();
			for( unsigned j = 0; j < bitmap->GetHeight(); ++j )
			{
				memcpy( reinterpret_cast<char*>( rect.pBits ) + j * rect.Pitch, row, bitmap->GetWidth() * 4 );
				row += bitmap->GetPitch();
			}
		}
		break;
	}
	m_cursor->UnlockRect();

	m_hotspotX = hotspotX;
	m_hotspotY = hotspotY;

	return true;
#elif defined(_WIN32)
    BITMAPV5HEADER bi;
    ZeroMemory( &bi, sizeof( BITMAPV5HEADER ) );
    bi.bV5Size = sizeof( BITMAPV5HEADER );
	bi.bV5Width = bitmap->GetWidth();
	bi.bV5Height = bitmap->GetHeight();
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask   =  0x00FF0000;
    bi.bV5GreenMask =  0x0000FF00;
    bi.bV5BlueMask  =  0x000000FF;
    bi.bV5AlphaMask =  0xFF000000; 
	bi.bV5CSType = LCS_WINDOWS_COLOR_SPACE;

    HDC hdc;
    hdc = GetDC( nullptr );

    void *bits;
    HBITMAP bmp = CreateDIBSection( hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, NULL, 0 );

    ReleaseDC( nullptr, hdc );

	if( bmp == nullptr )
	{
		CCP_LOGERR( "Tr2MouseCursor.Create: could not create DIB" );
		return false;
	}
	ON_BLOCK_EXIT( [&] { DeleteObject( bmp ); } );

	
	switch( bitmap->GetFormat() )
	{
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB:
		DecompressBC1( reinterpret_cast<char*>( bits ), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetWidth() * sizeof( uint32_t ) );
		break;

	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB:
		DecompressBC2( reinterpret_cast<char*>( bits ), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetWidth() * sizeof( uint32_t ) );
		break;
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		{
			const char* row = bitmap->GetRawData();
			for( unsigned j = 0; j < bitmap->GetHeight(); ++j )
			{
				memcpy( reinterpret_cast<uint32_t*>( bits ) + ( bitmap->GetHeight() - 1 - j ) * bitmap->GetWidth(), row, bitmap->GetWidth() * 4 );
				row += bitmap->GetPitch();
			}
		}
		break;
	}

	// Special case: fully transparent cursor (all alpha values are 0)
	// For such bitmap CreateIconIndirect function creates an incorrect
	// cursor without transparency
	bool allTransparent = true;
	for( unsigned i = 0; i < 32 * 32; ++i )
	{
		if( reinterpret_cast<char*>( bits )[i * 4 + 3] )
		{
			allTransparent = false;
			break;
		}
	}
	if( allTransparent )
	{
		BYTE andBits[32 * 4];
		BYTE xorBits[32 * 4];
		for( unsigned i = 0; i < 32 * 4; ++i )
		{
			xorBits[i] = 0;
			andBits[i] = 0xff;
		}
		m_cursor = CreateCursor( GetModuleHandle( nullptr ), hotspotX, hotspotY, 32, 32, andBits, xorBits );
		return m_cursor != nullptr;
	}

    // Create an empty mask bitmap.
    HBITMAP monoBitmap = CreateBitmap( bitmap->GetWidth(), bitmap->GetHeight(), 1, 1, nullptr );
	if( monoBitmap == nullptr )
	{
		CCP_LOGERR( "Tr2MouseCursor.Create: could not create alpha bitmap" );
		return false;
	}
	ON_BLOCK_EXIT( [&] { DeleteObject( monoBitmap ); } );

    ICONINFO ii;
    ii.fIcon = FALSE;
    ii.xHotspot = hotspotX;
    ii.yHotspot = hotspotY;
    ii.hbmMask = monoBitmap;
    ii.hbmColor = bmp;

    // Create the alpha cursor with the alpha DIB section.
    m_cursor = (HCURSOR)CreateIconIndirect( &ii );

	return m_cursor != nullptr;
#elif __APPLE__
    std::unique_ptr<char[]> bits( new char[bitmap->GetWidth() * bitmap->GetHeight() * 4] );
    switch( bitmap->GetFormat() )
    {
    case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS:
    case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM:
    case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB:
        DecompressBC1( bits.get(), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetWidth() * sizeof( uint32_t ) );
        break;

    case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS:
    case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM:
    case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB:
        DecompressBC2( bits.get(), bitmap->GetRawData(), bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetWidth() * sizeof( uint32_t ) );
        break;
    case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS:
    case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
    case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
        {
            const char* row = bitmap->GetRawData();
            for( uint32_t j = 0; j < bitmap->GetHeight(); ++j )
            {
                memcpy( bits.get() + j * bitmap->GetWidth() * 4, row, bitmap->GetWidth() * 4 );
                row += bitmap->GetPitch();
            }
        }
        break;
    default:
        return false;
    }
    return Create_MacOS( bits.get(), bitmap->GetWidth(), bitmap->GetHeight(), hotspotX, hotspotY );
#else
    return false;
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies the cursor to the mouse pointer.
// --------------------------------------------------------------------------------------
void Tr2MouseCursor::Apply()
{
	if( !IsValid() )
	{
		return;
	}
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( renderContext.m_d3dDevice9 )
	{
		renderContext.m_d3dDevice9->SetCursorProperties( m_hotspotX, m_hotspotY, m_cursor );
	}
#elif defined(_WIN32)
	SetCursor( m_cursor );
#elif __APPLE__
    Apply_MacOS();
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Destroys cursor surface for DX9.
// Arguments:
//   s - Memory class
// --------------------------------------------------------------------------------------
void Tr2MouseCursor::ReleaseResources( TriStorage s )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		m_cursor = nullptr;
	}
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Does nothing: we don't auto-restore cursors.
// Return Value:
//   true Always
// --------------------------------------------------------------------------------------
bool Tr2MouseCursor::OnPrepareResources()
{
	return true;
}
