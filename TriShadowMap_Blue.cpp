#include "StdAfx.h"
#include "TriShadowMap.h"

BLUE_DEFINE( TriShadowMap );

const Be::ClassInfo* TriShadowMap::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriShadowMap, "" )
		MAP_INTERFACE( TriShadowMap )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Disable shadows (this is just for debugging!)", Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( "size", m_size, "The size of the shadow map, is always quadric", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "depthBias", m_depthBias, "Depth bias for the VSM always-lit test", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "lightLeakStep", m_lightLeakStep, "Cut off value to combat light leaking", Be::READWRITE | Be::NOTIFY )		
		MAP_ATTRIBUTE( "filterVsm", m_filterVsm, "Filter the VSM", Be::READWRITE | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "debugFreezeObb", m_debugFreezeObb, "Debug the OBB by freezing it", Be::READWRITE )
	EXPOSURE_END()
}
