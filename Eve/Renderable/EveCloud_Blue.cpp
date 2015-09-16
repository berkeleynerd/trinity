////////////////////////////////////////////////////////////
//
//    Created:   February 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveCloud.h"
#include "EveCloudEditableVolume.h"

BLUE_DEFINE( EveCloud );

const Be::ClassInfo* EveCloud::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveCloud, "Cloud space object" )
        MAP_INTERFACE( EveCloud )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( ITr2GeometryProvider )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2SecondaryLightSource )

		MAP_ATTRIBUTE( "name", m_name, "The name of the cloud", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle display", Be::READWRITE )
		MAP_ATTRIBUTE( "sortingModifier", m_sortingModifier, "Affects the transparency sorting", Be::READWRITE )

		MAP_ATTRIBUTE( "effect", m_effect, "Shader used for the rendering", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE
		(    
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of a cloud",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of a cloud",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE( "scaling", m_scaling, "Object scaling", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "Used for culling", Be::READ )
		MAP_ATTRIBUTE( "preTesselationLevel", m_preTesselationLevel, "Number of triangles per width/heigth", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "volume", m_volume, "Shape volume texture editor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"secondaryLightingSphereRadius", 
			m_secondaryLightingSphereRadiusLocal, 
			"Radius of secondary light source", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"secondaryLightingEmissiveColor", 
			m_secondaryLightingEmissiveColor, 
			"Color of the secondary light source", 
			Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}
