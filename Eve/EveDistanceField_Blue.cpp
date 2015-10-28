#include "StdAfx.h"
#include "EveDistanceField.h"

BLUE_DEFINE( EveDistanceField );

const Be::ClassInfo* EveDistanceField::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistanceField, "" )
        MAP_INTERFACE( EveDistanceField )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "cameraView", m_cameraView, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "objects", m_objects, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "curveSet", m_curveSet, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distance", m_distance, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "timeAdjustmentSecondsOut", 
						m_timeAdjustmentSecondsOut, 
						"Adjust how long it takes to settle on a new value when zooming out", 
						Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "timeAdjustmentSecondsIn", 
						m_timeAdjustmentSecondsIn, 
						"Adjust how long it takes to settle on a new value when zooming in", 
						Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "dimensions", m_dimensions, "", Be::READ );
		MAP_ATTRIBUTE( "midpoint", m_middle, "", Be::READ );
		MAP_ATTRIBUTE( "distanceThreshold", m_distanceThreshold, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "maxXZRatio", m_maxXZRatio, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "minYRatio", m_minYRatio, "", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}