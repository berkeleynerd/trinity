////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2TranslationAdapter.h"


BLUE_DEFINE( Tr2TranslationAdapter );

const Be::ClassInfo* Tr2TranslationAdapter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TranslationAdapter, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2TranslationAdapter )
		MAP_INTERFACE( ITriVectorFunction )

		MAP_ATTRIBUTE(
			"value",
			m_value,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"curve",
			m_curve,
			"Attached curve",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

	EXPOSURE_END()
}