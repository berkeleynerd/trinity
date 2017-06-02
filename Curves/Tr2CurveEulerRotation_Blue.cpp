////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveEulerRotation.h"

BLUE_DEFINE( Tr2CurveEulerRotation );

const Be::ClassInfo* Tr2CurveEulerRotation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveEulerRotation, ":jessica-icon: tree/trirotationcurve.png" )
		MAP_INTERFACE( Tr2CurveEulerRotation )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"yaw",
			m_yaw,
			"Yaw component curve",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"pitch",
			m_pitch,
			"Pitch component curve",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"roll",
			m_roll,
			"Pitch component curve",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time"
		)

	EXPOSURE_END()
}