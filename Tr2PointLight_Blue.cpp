#include "StdAfx.h"
#include "Tr2PointLight.h"

BLUE_DEFINE( Tr2PointLight );

const Be::ClassInfo* Tr2PointLight::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2PointLight, "" )
        MAP_INTERFACE( Tr2PointLight )

		MAP_ATTRIBUTE( "name", m_name, "Name so artists dont get confused", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Light position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_radius, "Light radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "Light color (in linear space)\n:jessica-tuple-type: linearcolor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_brightness, "Light brightness (modulates color) for easier animation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "noiseAmplitude", m_noiseAmplitude, "Brightness noise amplitude\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_noiseFrequency, "Brightness noise frequency\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_noiseOctaves, "Brightness turbulence octaves\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
