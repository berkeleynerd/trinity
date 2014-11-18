////////////////////////////////////////////////////////////
//
//    Created:   November 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveSceneStaticParticles.h"

BLUE_DEFINE( EveSceneStaticParticles );

const Be::ClassInfo* EveSceneStaticParticles::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSceneStaticParticles, "" )
        MAP_INTERFACE( EveSceneStaticParticles )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "maxParticleCount", m_maxParticleCount, "n/a", Be::READWRITE )
		MAP_ATTRIBUTE( "minSize", m_minSize, "n/a", Be::READWRITE )
		MAP_ATTRIBUTE( "maxSize", m_maxSize, "n/a", Be::READWRITE )

    EXPOSURE_END()
}
