////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveDustfieldConstraint.h"

BLUE_DEFINE( EveDustfieldConstraint );

const Be::ClassInfo* EveDustfieldConstraint::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDustfieldConstraint, "" )
        MAP_INTERFACE( EveDustfieldConstraint )
		MAP_INTERFACE( ITr2GenericParticleConstraint )

		MAP_ATTRIBUTE( "cameraView", m_cameraView, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "referencePosition", m_referencePosition, "Center of the dustfield sphere", Be::READ )
		MAP_ATTRIBUTE( "velocityStretch", m_velocityStretch, "Velocity stretch effect parameter", Be::READ )

		MAP_ATTRIBUTE( "radius", m_radius, "Effect sphere radius", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"stretch", 
			m_stretch, 
			"Stretch factor multiplied with the reference point's speed, used to "
			"calculate the magnitude of the fake motion blur stretch", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxStretch", m_maxStretch, "", Be::READWRITE | Be::PERSIST  )

		MAP_ATTRIBUTE( 
			"distanceFunction", 
			m_distanceFunction, 
			"Scalar function that's driven by the eye's distance from lookat point", 
			Be::READWRITE | Be::PERSIST  )
		MAP_ATTRIBUTE( 
			"speedFunction",
			m_speedFunction, 
			"Scalar function that's driven by speed", 
			Be::READWRITE | Be::PERSIST  )
		MAP_ATTRIBUTE( "movementScale", m_movementScale, "Scale how much particles move(applied after max speed)", Be::READWRITE | Be::PERSIST  )
		MAP_ATTRIBUTE( "maxSpeed", m_maxSpeed, "Clamp movement of particles to this value", Be::READWRITE | Be::PERSIST  )

	EXPOSURE_END()
}