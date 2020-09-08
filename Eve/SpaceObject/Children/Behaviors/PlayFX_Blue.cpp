#include "StdAfx.h"
#include "PlayFX.h"

BLUE_DEFINE( PlayFX );

const Be::ClassInfo* PlayFX::ExposeToBlue()
{
	EXPOSURE_BEGIN( PlayFX, "" )
		MAP_INTERFACE( PlayFX )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, ":jessica-group: PlayFX", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sec", m_sec, "How long should the fx play", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "A stretch effect for this firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingEffects", m_firingEffects, "A stretch effect for this firing effect", Be::READ )
	
	EXPOSURE_END()
}