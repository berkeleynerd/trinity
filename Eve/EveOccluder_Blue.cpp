////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveOccluder.h"

BLUE_DEFINE( EveOccluder );

const Be::ClassInfo* EveOccluder::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveOccluder, "" )
        MAP_INTERFACE( EveOccluder )

		MAP_ATTRIBUTE( "name", m_name, "A name for this occluder", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "value", m_value, "this is the result: how much is visible?", Be::READ )
		MAP_ATTRIBUTE( "totalNumOfPixels", m_totalNumOfPixels, "reference value: how many pixels got rendered without depth testing", Be::READ )
		MAP_ATTRIBUTE( "actualNumOfPixels", m_actualNumOfPixels, "test value: how many pixels got rendered with depth testing", Be::READ )

		MAP_ATTRIBUTE( "sprites", m_sprites, "a list of sprites for the occlusion query", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
