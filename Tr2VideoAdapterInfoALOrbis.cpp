#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2VideoAdapterInfoALOrbis.h"
#include "ALLog.h"
#include <video_out.h>

using namespace Tr2RenderContextEnum;

namespace
{
static int32_t s_videoHandle = -1;

bool InitializeVideo()
{
	if( s_videoHandle < 0 )
	{
		s_videoHandle = sceVideoOutOpen( SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL );
		if( s_videoHandle < 0 )
		{
			CCP_AL_LOGERR( "Tr2VideoAdapterInfo: sceVideoOutOpen returned error" );
			return false;
		}
	}
	return true;
}

}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	if( !InitializeVideo() )
	{
		return E_FAIL;
	}
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex, Tr2AdapterInfo& info )
{
	if( adapterIndex > 0 )
	{
		return E_INVALIDARG;
	}
	if( !InitializeVideo() )
	{
		return E_FAIL;
	}
	info.description = L"";
	info.deviceID = 0;
	memset( &info.deviceIdentifier, 0, sizeof( info.deviceIdentifier ) );
	info.deviceName = "";
	info.driver = "";
	info.driverVersion = 0;
	info.revision = 0;
	info.subSystemID = 0;
	info.vendorID = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex, void*& monitor )
{
	if( adapterIndex > 0 )
	{
		return E_INVALIDARG;
	}
	if( !InitializeVideo() )
	{
		return E_FAIL;
	}
	monitor = nullptr;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex, Tr2DisplayModeInfo& mode )
{
	if( adapterIndex > 0 )
	{
		return E_INVALIDARG;
	}
	if( !InitializeVideo() )
	{
		return E_FAIL;
	}
	SceVideoOutResolutionStatus resolution;
	if( sceVideoOutGetResolutionStatus( s_videoHandle, &resolution ) < 0 )
	{
		return E_FAIL;
	}
	mode.width = resolution.fullWidth;
	mode.height = resolution.fullHeight;
	switch( resolution.refreshRate )
	{
	case SCE_VIDEO_OUT_REFRESH_RATE_23_98HZ:
		mode.refreshRateNumerator = 100;
		mode.refreshRateDenominator = 2398;
		break;
	case SCE_VIDEO_OUT_REFRESH_RATE_50HZ:
		mode.refreshRateNumerator = 1;
		mode.refreshRateDenominator = 50;
		break;
	case SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ:
		mode.refreshRateNumerator = 100;
		mode.refreshRateDenominator = 5994;
		break;
	default:
		mode.refreshRateNumerator = 0;
		mode.refreshRateDenominator = 0;
	}
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.format = PIXEL_FORMAT_B8G8R8A8_UNORM;

	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat backBufferFormat, unsigned& count )
{
	if( adapterIndex > 0 )
	{
		return E_INVALIDARG;
	}
	if( !InitializeVideo() )
	{
		return E_FAIL;
	}
	if( backBufferFormat != PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		count = 0;
		return S_OK;
	}
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat backBufferFormat, unsigned modeIndex, Tr2DisplayModeInfo& mode )
{
	if( adapterIndex > 0 || backBufferFormat != PIXEL_FORMAT_B8G8R8A8_UNORM || modeIndex > 0 )
	{
		return E_INVALIDARG;
	}
	return GetAdapterDisplayMode( 0, mode );
}

int32_t Tr2VideoAdapterInfo::InternalGetVideoHandle()
{
	if( !InitializeVideo() )
	{
		return -1;
	}
	return s_videoHandle;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned adapterIndex, unsigned& version )
{
	if( adapterIndex > 0 )
	{
		return E_INVALIDARG;
	}
	version = 5 << 8;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat backBufferFormat, bool windowed )
{
	if( adapterIndex > 0 || backBufferFormat != PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		return false;
	}
	return true;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1, unsigned adapter2 )
{
	return adapter1 != adapter2;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
									  Tr2RenderContextEnum::PixelFormat format,
									  bool windowed,
									  unsigned msaaType,
									  unsigned& msaaQuality )
{
	return E_INVALIDCALL;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
									  Tr2RenderContextEnum::DepthStencilFormat format,
									  bool windowed,
									  unsigned msaaType,
									  unsigned& msaaQuality )
{
	return E_INVALIDCALL;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
									   Tr2RenderContextEnum::PixelFormat backBufferFormat,
									   Tr2RenderContextEnum::PixelFormat format,
									   bool withAutoGenMipmap )
{
	return false;
}
bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
									   Tr2RenderContextEnum::PixelFormat backBufferFormat,
									   Tr2RenderContextEnum::DepthStencilFormat format )
{
	return false;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned adapterIndex,
										  unsigned& maxWidth )
{
	return E_INVALIDCALL;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return E_INVALIDCALL;
}

#endif