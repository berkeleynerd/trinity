////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//
#include "StdAfx.h"
#include "EveUiObject.h"

BLUE_DEFINE( EveUiObject );

const Be::ClassInfo* EveUiObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveUiObject, "" )
		MAP_INTERFACE( EveUiObject )
		MAP_INTERFACE( IEveSpaceObject2 )

		MAP_ATTRIBUTE( "usePerspectiveScale", m_usePerspectiveScale, "Use distance-based scaling (perspective)", Be::READWRITE );

	EXPOSURE_CHAINTO( EveSpaceObject2 )
}