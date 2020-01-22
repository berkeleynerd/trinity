#include "StdAfx.h"
#include "PlayFX.h"

//BLUE_DEFINE_INTERFACE( IEveFiringEffectElement );

BLUE_DEFINE( PlayFX );

const Be::ClassInfo* PlayFX::ExposeToBlue()
{
	EXPOSURE_BEGIN( PlayFX, "" )
		MAP_INTERFACE( PlayFX )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, ":jessica-group: PlayFX", Be::READWRITE )
		MAP_ATTRIBUTE( "low", m_low, "Lower value of a randomized timing for drones to play effect", Be::READWRITE )
		MAP_ATTRIBUTE( "high", m_high, "Higher value of a randomized timing for drones to play effect", Be::READWRITE )
		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "A stretch effect for this firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingEffects", m_firingEffects, "A stretch effect for this firing effect", Be::READ )

	EXPOSURE_END()
}