#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

const char ERRNODEVICE[] = "There is no D3D device available";

#include "TriDevice.h"
#include "TriError.h"
#include "RenderJob/Tr2RenderJobs.h"

using namespace Tr2RenderContextEnum;

#include "dxerr.h"

CCP_STATS_DECLARED_ELSEWHERE( presentTime );

void TriDevice::UpdateCursor()
{	
#if BLUE_WITH_PYTHON
	if (!mPresentParam.windowed)
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		static POINT opt = {-1,-1};
		POINT pt;		
		GetCursorPos(&pt);
		if((pt.x != opt.x) || (pt.y != opt.y))
		{
			opt = pt;
			if( renderContext.m_d3dDevice9 )
			{
				renderContext.m_d3dDevice9->SetCursorPosition(pt.x, pt.y, D3DCURSOR_IMMEDIATE_UPDATE);
			}
		}
	}
#endif
}

void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
	AutoTasklet _at( PyOS->GetTaskletTimer(), "TriDevice::HandleRenderTick" );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !renderContext.m_d3dDevice9 )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
	}

	HRESULT hr = renderContext.TestCooperativeLevel();
	switch( hr )
	{
	case E_DEVICELOST:
		if( !mDeviceLost )
		{
			ReleaseDeviceResources( TRISTORAGE_VIDEOMEMORY );
			mDeviceLost = true;
			return; // There's no point in continuing along this function now...
		}
		break;

	case D3DERR_DEVICENOTRESET:
		// Device coming back from lost state.
		if( !ResetDevice() )
		{
			return; //big problem.  we are probably lost here.  
		}
		mDeviceLost = false;
		break;

	case D3DERR_DRIVERINTERNALERROR:
		TriError::ReportError(hr, Clsid(), "TestCooperativeLevel failed");
		ResetDevice();
		return;

	default:
		if( FAILED( hr ) )
		{
			TriError::ReportError(hr, Clsid(), "TestCooperativeLevel failed");
			return;
		}
	}
    if( mDeviceLost )
	{
		return;
	}

	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( realTime, simTime );
	}
	
	m_postUpdateCallbacks->Update();

	// Present the backbuffer from the last renderering to the front buffer.
	// it is more efficient to do it like this (revers order), because there is some
	// acyncrounicy between EndScene() and Present(). So if we pump Python
	// and do all the other stuff between we get a degree of paralization
	{
		CCP_STATS_SCOPED_TIME( presentTime );
		CR_RETURN( renderContext.Present() );
	}
		
	if( !Render() )
	{
		REPORTERROR( "Failed to render a frame" );
	}
}

// -- Smaller helpers to enable big methods like TriDevice::Render to be mostly API neutral.

// Do we have a valid device pointer? Lower level question than "do we have a valid renderContext",
// so first question may be true while second one is still false.
bool TriDevice::DeviceExists()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.m_d3dDevice9 != nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   A chance for device to respond to application window being activated/deactivated. 
//   Not implemented for DX9 - we are happy with how windows behaves in DX9.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation )
{
}

#endif
