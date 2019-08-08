#include "StdAfx.h"
#include "Inertia.h"

BLUE_DEFINE( Inertia );

const Be::ClassInfo* Inertia::ExposeToBlue()
{
	EXPOSURE_BEGIN( Inertia, "" )
		MAP_INTERFACE( Inertia )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "inertiaWeight", m_inertiaWeight, ":jessica-group: Inertia", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRotationSpeed", m_maxRotationSpeed, ":jessica-group: Inertia", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxAcceleration", m_maxAcceleration, ":jessica-group: Inertia", Be::READWRITE | Be::PERSIST )

		EXPOSURE_END()
}