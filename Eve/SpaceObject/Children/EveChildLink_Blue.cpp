////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveChildLink.h"

BLUE_DEFINE( EveChildLink );

const Be::ClassInfo* EveChildLink::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildLink, "" )
        MAP_INTERFACE( EveChildLink )
        MAP_INTERFACE( EveChildMesh )

		MAP_ATTRIBUTE( "scale", m_scale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "direction", m_direction, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linkStrength", m_linkStrength, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linkStrengthCurves", m_linkStrengthCurves, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linkStrengthBindings", m_linkStrengthBindings, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_CHAINTO( EveChildMesh )
}