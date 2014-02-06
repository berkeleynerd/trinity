#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2VideoAdapterInfoALGLES2.h"

#ifndef _WIN32

#if defined(TRINITY_AL_MOBILE)
ALResult Tr2VideoAdapterInfo::InternalInitializeEgl()
{
	static bool initialized = false;
	if( !initialized )
	{
		auto display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
		if( display == EGL_NO_DISPLAY )
		{
			return E_FAIL;
		}
		if( !eglInitialize( display, nullptr, nullptr ) )
		{
			return E_FAIL;
		}
		initialized = true;
	}
	return S_OK;
}
#endif

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex, Tr2AdapterInfo& info )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	info.driver = "na";
	info.description = L"na";
	info.deviceName = "defaultDevice";
	info.driverVersion = 0;
	info.vendorID = 0;
	info.deviceID = 0;
	info.subSystemID = 0;
	info.revision = 0;
    memset( &info.deviceIdentifier, 0, sizeof( info.deviceIdentifier ) );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex, void*& monitor )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
#if defined(TRINITY_AL_MOBILE)
	auto display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	if( display == EGL_NO_DISPLAY )
	{
		return E_FAIL;
	}
	monitor = display;
#else
	monitor = nullptr;
#endif
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex, Tr2DisplayModeInfo& mode )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
#if defined(TRINITY_AL_MOBILE)
	static EGLint width = 0;
	static EGLint height = 0;
	if( !width )
	{
		const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE };
 
		InternalInitializeEgl();
		auto display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

		EGLConfig config;
		EGLint numConfigs;
    
		if( !eglChooseConfig( display, attribs, &config, 1, &numConfigs ) )
		{
			return E_FAIL;
		}

		extern ANativeWindow* g_androidWindow;
		EGLSurface surface = eglCreateWindowSurface( display, config, g_androidWindow, nullptr );
		ON_BLOCK_EXIT( [&] { eglDestroySurface( display, surface ); } );
    
		if( !eglQuerySurface( display, surface, EGL_HEIGHT, &height ) )
		{
			return E_FAIL;
		}
		if( !eglQuerySurface( display, surface, EGL_WIDTH, &width ) )
		{
			return E_FAIL;
		}
	}
	mode.width = width;
	mode.height = height;
#else
	mode.width = 1024;
	mode.height = 768;
#endif
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = 60;
	mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	mode.scanlineOrdering = Tr2RenderContextEnum::SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = Tr2RenderContextEnum::DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	unsigned& count )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	count = backBufferFormat == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM ? 1 : 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	unsigned modeIndex,
	Tr2DisplayModeInfo& mode )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	if( backBufferFormat != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		return E_INVALIDARG;
	}
	if( modeIndex )
	{
		return E_INVALIDARG;
	}

	mode.width = 1024;
	mode.height = 768;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = 60;
	mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	mode.scanlineOrdering = Tr2RenderContextEnum::SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = Tr2RenderContextEnum::DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat format,
	bool windowed,
	unsigned msaaType,
	unsigned& msaaQuality )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::DepthStencilFormat format,
	bool windowed,
	unsigned msaaType,
	unsigned& msaaQuality )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( 
	unsigned adapterIndex,
	unsigned& version )
{
	version = 3 << 8;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	bool windowed )
{
	if( adapterIndex )
	{
		return false;
	}
	return backBufferFormat == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::PixelFormat format,
	bool withAutoGenMipmap )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::DepthStencilFormat format )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::PixelFormat format )
{
	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( 
	unsigned adapterIndex,
	unsigned& maxWidth )
{
	maxWidth = 4096;
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1, unsigned adapter2 )
{
	return false;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return S_OK;
}

#else

extern bool g_usingEXDevice;
extern std::vector<HANDLE> g_D3DCreatedHeaps;

using namespace Tr2RenderContextEnum;

namespace
{
CComPtr<IDirect3D9> s_direct3D;

ALResult InitializeDirect3D()
{
	s_direct3D = nullptr;

	//
	// First see if we the extended device is supported
	//
	g_usingEXDevice = false;

	if ( !s_direct3D )
	{
		IDirect3D9* d3d = NULL;
		d3d = Direct3DCreate9( D3D_SDK_VERSION );
		if ( !d3d )
		{
			CCP_LOGERR( "Failed to create the Direct3D object" );
			g_usingEXDevice = false;
		}
		CCP_LOG( "Trinity is using the regular device" );
		s_direct3D.Attach( d3d );
	}

	if ( !s_direct3D )
	{
		return E_FAIL;
	}
	if ( !D3DXCheckVersion( D3D_SDK_VERSION, D3DX_SDK_VERSION ) )
	{
		s_direct3D.Release();
		return E_FAIL;
	}
	return S_OK;
}

}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	count = s_direct3D->GetAdapterCount();
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DADAPTER_IDENTIFIER9 id;
	CR_RETURN_HR( s_direct3D->GetAdapterIdentifier( adapterIndex, 0, &id ) );
	info.driver = id.Driver;
	info.description = CA2W( id.Description );
	info.deviceName = id.DeviceName;
	info.driverVersion = id.DriverVersion.QuadPart;
	info.vendorID = id.VendorId;
	info.deviceID = id.DeviceId;
	info.subSystemID = id.SubSysId;
	info.revision = id.Revision;
	info.deviceIdentifier = *reinterpret_cast<AdapterGuid*>( &id.DeviceIdentifier );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	monitor = s_direct3D->GetAdapterMonitor( adapterIndex );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DDISPLAYMODE id;
	CR_RETURN_HR( s_direct3D->GetAdapterDisplayMode( adapterIndex, &id ) );
	mode.width = id.Width;
	mode.height = id.Height;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = id.RefreshRate;
	mode.format = Tr2RenderContextEnum::ConvertFromD3D9Format( id.Format );
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat backBufferFormat,
												   unsigned& count )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}

	D3DFORMAT format = Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat );

	static const D3DFORMAT allowed[] =
	{
		D3DFMT_X8R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A2R10G10B10,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_R5G6B5
	};
	const int nAllowed = sizeof( allowed ) / sizeof( allowed[0] );
	int i;

	for ( i = 0; i < nAllowed; i++ )
	{
		if ( format == allowed[i] )
		{
			break;
		}
	}
	if ( i == nAllowed )
	{
		return E_FAIL;
	}

	count = s_direct3D->GetAdapterModeCount( adapterIndex, format );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat backBufferFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DDISPLAYMODE id;
	CR_RETURN_HR( s_direct3D->EnumAdapterModes( adapterIndex, Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat ), modeIndex, &id ) )
	;
	mode.width = id.Width;
	mode.height = id.Height;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = id.RefreshRate;
	mode.format = Tr2RenderContextEnum::ConvertFromD3D9Format( id.Format );
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned adapterIndex,
													   unsigned& version )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DCAPS9 caps;
	CR_RETURN_HR( s_direct3D->GetDeviceCaps( adapterIndex, D3DDEVTYPE_HAL, &caps ) );
	version = std::min( caps.VertexShaderVersion, caps.PixelShaderVersion );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned adapterIndex,
														 unsigned& maxWidth )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DCAPS9 caps;
	CR_RETURN_HR( s_direct3D->GetDeviceCaps( adapterIndex, D3DDEVTYPE_HAL, &caps ) );
	maxWidth = caps.MaxTextureWidth;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat,
													bool windowed )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	D3DFORMAT bbFormat = Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat );
	D3DFORMAT displayFormat;
	switch ( bbFormat )
	{
	case D3DFMT_A8R8G8B8:
		displayFormat = D3DFMT_X8R8G8B8;
		break;
	case D3DFMT_A1R5G5B5:
		displayFormat = D3DFMT_X1R5G5B5;
		break;
	default:
		displayFormat = bbFormat;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceType( adapterIndex,
		D3DDEVTYPE_HAL,
		displayFormat,
		bbFormat,
		windowed ? TRUE : FALSE ) );
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::PixelFormat format,
													  bool withAutoGenMipmap )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_RENDERTARGET | ( withAutoGenMipmap ? D3DUSAGE_AUTOGENMIPMAP : 0 ),
		D3DRTYPE_SURFACE,
		Tr2RenderContextEnum::ConvertToD3D9Format( format ) ) );
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::DepthStencilFormat format )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	D3DFORMAT format9 = Tr2RenderContextEnum::ConvertToD3D9DepthStencilFormat( format );
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_DEPTHSTENCIL,
		D3DRTYPE_SURFACE,
		format9 ) );
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( unsigned adapterIndex,
													   Tr2RenderContextEnum::PixelFormat backBufferFormat,
													   Tr2RenderContextEnum::PixelFormat format )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextEnum::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_QUERY_VERTEXTEXTURE,
		D3DRTYPE_SURFACE,
		Tr2RenderContextEnum::ConvertToD3D9Format( format ) ) );
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::PixelFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	const auto
	sample = static_cast<D3DMULTISAMPLE_TYPE>( msaaType > 1 ? msaaType : 0 );
	DWORD levels = 0;
	CR_RETURN_HR( s_direct3D->CheckDeviceMultiSampleType( adapterIndex, D3DDEVTYPE_HAL, Tr2RenderContextEnum::ConvertToD3D9Format( format ), windowed ? TRUE : FALSE, sample, &levels ) )
	;
	msaaQuality = levels;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::DepthStencilFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DFORMAT format9 = Tr2RenderContextEnum::ConvertToD3D9DepthStencilFormat( format );
	const auto
	sample = static_cast<D3DMULTISAMPLE_TYPE>( msaaType > 1 ? msaaType : 0 );
	DWORD levels = 0;
	CR_RETURN_HR( s_direct3D->CheckDeviceMultiSampleType( adapterIndex, D3DDEVTYPE_HAL, format9, windowed ? TRUE : FALSE, sample, &levels ) )
	;
	msaaQuality = levels;
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
												unsigned adapter2 )
{
	return adapter1 != adapter2;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return InitializeDirect3D();
}

#endif

#endif
