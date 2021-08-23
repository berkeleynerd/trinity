////////////////////////////////////////////////////////////
//
//    Created:   September 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2VideoAdapters.h"
#include "TriSettingsRegistrar.h"
#include "TrinityAL/Tr2DriverUtilities.h"

#ifdef _WIN32
std::vector<HANDLE> g_D3DCreatedHeaps;
bool g_usingEXDevice = false;
TRI_REGISTER_SETTING( "usingEXDevice", g_usingEXDevice );

#endif

extern bool g_wantsEXDevice;

namespace
{

struct InitializeWantsEXDevice
{
	InitializeWantsEXDevice()
	{
		g_wantsEXDevice = BeOS->HasStartupArg( L"/enableExDevice" );

#ifdef _WIN32
		char env[64];
		size_t size = 0;
		getenv_s( &size, env, "TRINITYEXDEVICE" );
		if( size != 0 && strcmp( env, "0" ) != 0 )
		{
			g_wantsEXDevice = true;
		}
#endif
	}
};

}

using namespace Tr2RenderContextEnum;

Tr2VideoAdapters::Tr2VideoAdapters()
	:DEFAULT_ADAPTER( Tr2VideoAdapterInfo::DEFAULT_ADAPTER ) 
{
	static InitializeWantsEXDevice doInitializeWantsEXDevice;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query number of display adapters (video cards) available.
// Arguments:
//   count - (out) Number of adapters available
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetAdapterCount( unsigned& count )
{
	return Tr2VideoAdapterInfo::GetAdapterCount( count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get display adapter properties.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   info - (out) Tr2VideoAdapter object with adapter properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetAdapterInfo( unsigned adapterIndex,
										   Tr2VideoAdapter** info )
{
	*info = nullptr;
	Tr2AdapterInfo adapterInfo;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterInfo( adapterIndex, adapterInfo ) );
	Tr2VideoAdapterPtr adapter;
	adapter.CreateInstance();
	adapter->m_index = adapterIndex;
	adapter->m_info = adapterInfo;
	*info = adapter.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get current display mode for given display adapter.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   mode - (out) Tr2DisplayMode object with display mode properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetCurrentDisplayMode( unsigned adapterIndex,
												  Tr2DisplayMode** mode )
{
	*mode = nullptr;
	Tr2DisplayModeInfo info;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterDisplayMode( adapterIndex, info ) );
	Tr2DisplayModePtr result;
	result.CreateInstance();
	result->m_mode = info;
	*mode = result.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get number of available display modes supported by display adapter for the given 
//   back buffer format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   count - (out) number of display modes supported
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetDisplayModeCount( unsigned adapterIndex,
												int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
												unsigned& count )
{
	return Tr2VideoAdapterInfo::GetAdapterModeCount( adapterIndex, PixelFormat( backBufferFormat ), count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get display mode properties.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   modeIndex - display mode index (from 0 to GetDisplayModeCount)
//   mode - (out) Tr2DisplayMode object with display mode properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetDisplayMode( unsigned adapterIndex,
										   int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
										   unsigned modeIndex,
										   Tr2DisplayMode** mode )
{
	*mode = nullptr;
	Tr2DisplayModeInfo info;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterMode( adapterIndex, PixelFormat( backBufferFormat ), modeIndex, info ) )
	;
	Tr2DisplayModePtr result;
	result.CreateInstance();
	result->m_mode = info;
	*mode = result.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query MSAA render target support.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   format - render target format
//   windowed - if the application runs in windowed or fullscreen mode
//   msaaType - MSAA type (number of samples)
//   msaaQuality - (out) max MSAA quality (0 if MSAA type is unsupported)
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetRenderTargetMsaaSupport( unsigned adapterIndex,
													   int /*Tr2RenderContextEnum::PixelFormat*/ format,
													   bool windowed,
													   unsigned msaaType,
													   unsigned& msaaQuality )
{
	return Tr2VideoAdapterInfo::GetAdapterMsaaSupport( adapterIndex, PixelFormat( format ), windowed, msaaType,
		msaaQuality );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query MSAA depth stencil buffer support.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   format - depth stencil format
//   windowed - if the application runs in windowed or fullscreen mode
//   msaaType - MSAA type (number of samples)
//   msaaQuality - (out) max MSAA quality (0 if MSAA type is unsupported)
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetDepthStencilMsaaSupport( unsigned adapterIndex,
													   int /*Tr2RenderContextEnum::DepthStencilFormat*/ format,
													   bool windowed,
													   unsigned msaaType,
													   unsigned& msaaQuality )
{
	return Tr2VideoAdapterInfo::GetAdapterMsaaSupport( adapterIndex, DepthStencilFormat( format ), windowed, msaaType,
		msaaQuality );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query maximum supported shader version.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   version - (out) shader version with major version in integer part andminor version 
//		in fractional part
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetShaderVersion( unsigned adapterIndex,
											 float& version )
{
	unsigned rawVersion = 0;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterShaderVersion( adapterIndex, rawVersion ) );
	version = ( ( rawVersion >> 8 ) & 0xff ) + float( rawVersion & 0xff ) / 10.f;
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if display adapter supports given back buffer format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   windowed - if the application runs in windowed or fullscreen mode
// Return Value:
//   true if adapter supports given back buffer format
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2VideoAdapters::SupportsBackBufferFormat( unsigned adapterIndex,
												 int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
												 bool windowed )
{
	return Tr2VideoAdapterInfo::SupportsBackBufferFormat( adapterIndex, PixelFormat( backBufferFormat ), windowed );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if display adapter supports given render target format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   format - render target format
//   windowed - if the application runs in windowed or fullscreen mode
// Return Value:
//   true if adapter supports given render target format
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2VideoAdapters::SupportsRenderTargetFormat( unsigned adapterIndex,
												   int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
												   int /*Tr2RenderContextEnum::PixelFormat*/ format )
{
	return Tr2VideoAdapterInfo::SupportsRenderTargetFormat( adapterIndex, PixelFormat( backBufferFormat ),
		PixelFormat( format ), false );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if display adapter supports given depth stencil format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   format - depth stencil format
//   windowed - if the application runs in windowed or fullscreen mode
// Return Value:
//   true if adapter supports given depth stencil format
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2VideoAdapters::SupportsDepthStencilFormat( unsigned adapterIndex,
												   int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
												   int /*Tr2RenderContextEnum::DepthStencilFormat*/ format )
{
	return Tr2VideoAdapterInfo::SupportsDepthStencilFormat( adapterIndex, PixelFormat( backBufferFormat ),
		DepthStencilFormat( format ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query maximum texture size (with or height) in pixels.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   maxWidth - (out) maximum texture size
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetMaxTextureSize( unsigned adapterIndex,
											  unsigned& maxWidth )
{
	return Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( adapterIndex, maxWidth );
}

// --------------------------------------------------------------------------------------
// Description:
//   Refreshes any cached information about video adapters.
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::RefreshData()
{
	return Tr2VideoAdapterInfo::RefreshData();
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to return device identifier GUID as a string.
// Return Value:
//   Device identifier GUID as a string
// --------------------------------------------------------------------------------------
std::string Tr2VideoAdapter::GetDeviceIdentifierString() const
{
#ifdef _WIN32
	LPOLESTR guidstr;
	if ( SUCCEEDED( StringFromCLSID( *reinterpret_cast<const GUID*>( &m_info.deviceIdentifier ), &guidstr ) ) )
	{
		CW2A str( guidstr );
		CoTaskMemFree( guidstr );
		return ( const char* )str;
	}
#else
    char buffer[128];
    sprintf(
            buffer,
            "%08x-%04x-%04x-%02xhh%02xhh-%02xhh%02xhh%02xhh%02xhh%02xhh%02xhh",
            unsigned( m_info.deviceIdentifier.data1 ),
            unsigned( m_info.deviceIdentifier.data2 ),
            unsigned( m_info.deviceIdentifier.data3 ),
            m_info.deviceIdentifier.data4[0],
            m_info.deviceIdentifier.data4[1],
            m_info.deviceIdentifier.data4[2],
            m_info.deviceIdentifier.data4[3],
            m_info.deviceIdentifier.data4[4],
            m_info.deviceIdentifier.data4[5],
            m_info.deviceIdentifier.data4[6],
            m_info.deviceIdentifier.data4[7] );
    return buffer;
#endif
	return "";
}

// --------------------------------------------------------------------------------------
// Description:
//   Get driver information for the display adapter.
// Arguments:
//   info - (out) Tr2VideoDriver object with video driver properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapter::GetDriverInfo( Tr2VideoDriver** info )
{
	*info = nullptr;
	
	Tr2VideoDriverInfo driverInfo;
	FORWARD_HR( Tr2DriverUtilities::GetDriverVersion( m_info.deviceID, driverInfo ) );

	Tr2VideoDriverPtr result;
	result.CreateInstance();
	result->m_info = driverInfo;
	*info = result.Detach();
	return S_OK;
}

