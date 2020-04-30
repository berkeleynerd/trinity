#include "StdAfx.h"
#include "CollisionAvoidance.h"

BLUE_DEFINE( CollisionAvoidance );

const Be::ClassInfo* CollisionAvoidance::ExposeToBlue()
{
	EXPOSURE_BEGIN( CollisionAvoidance, "" )
		MAP_INTERFACE( CollisionAvoidance )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "avoidanceScalar", m_collisionAvoidanceScalar, ":jessica-group: CollisionAvoidance", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}