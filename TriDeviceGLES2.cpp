#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 || TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "TriDevice.h"
#include "TriError.h"
#include "RenderJob/Tr2RenderJobs.h"

CCP_STATS_DECLARED_ELSEWHERE( presentTime );

void TriDevice::UpdateCursor() {}

void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !renderContext.IsValid() )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
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
		TriError::ReportError( BEDEF, Clsid(), "Failed to render a frame" );
	}
}

// -- Smaller helpers to enable big methods like TriDevice::Render to be mostly API neutral.

// Do we have a valid device pointer? Lower level question than "do we have a valid renderContext",
// so first question may be true while second one is still false.
bool TriDevice::DeviceExists()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   A chance for device to respond to application window being activated/deactivated. 
//   Not implemented for GL.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation )
{
}


#endif
