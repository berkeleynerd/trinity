////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

BLUE_DEFINE( EveImpactOverlay );

Be::VarChooser EveImpactCfgChooser[] =
{
	{
		"NONE", BeCast( EveImpactOverlay::IMPACT_INVALID ), "Invalid."
	},
	{
		"SHIELD", BeCast( EveImpactOverlay::IMPACT_SHIELD ), "Impact goes into shields."
	},
	{
		"ARMOR", BeCast( EveImpactOverlay::IMPACT_ARMOR ), "Impact goes into armor."
	},
	{
		"HULL", BeCast( EveImpactOverlay::IMPACT_HULL ), "Impact goes into hull."
	},
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( 
    "EveImpactCfg", 
	EveImpactOverlay::ImpactConfiguration, 
    EveImpactCfgChooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::ClassInfo* EveImpactOverlay::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveImpactOverlay, "" )
        MAP_INTERFACE( EveImpactOverlay )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE )
		MAP_ATTRIBUTE( "configuration", m_configuration, "Impact goes into what?", Be::READ | Be::ENUM )
		MAP_ATTRIBUTE( "impactDataNextIdx", m_impactDataNextIdx, "", Be::READ )

		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "dataTextureBlockID", m_dataTextureBlockID, "The ID for our part in the big texture.", Be::READ )

		MAP_ATTRIBUTE( "maxShieldImpacts", m_maxShieldImpacts, "", Be::READ )
		MAP_ATTRIBUTE( "shieldEllipsoidCenter", m_shieldEllipsoidCenter, "", Be::READ )
		MAP_ATTRIBUTE( "shieldEllipsoidRadii", m_shieldEllipsoidRadii, "", Be::READ )
		MAP_ATTRIBUTE( "overallShieldImpact", m_overallShieldImpact, "", Be::READWRITE )

		MAP_ATTRIBUTE( "armorDamageShader", m_armorDamageShader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorImpactSizeFactor", m_armorImpactSizeFactor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorImpactSizeMax", m_armorImpactSizeMax, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}