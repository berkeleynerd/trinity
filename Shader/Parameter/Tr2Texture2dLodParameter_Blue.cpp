////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2Texture2dLodParameter.h"

BLUE_DEFINE( Tr2Texture2dLodParameter );

const Be::ClassInfo* Tr2Texture2dLodParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Texture2dLodParameter, "Resource parameter for textures with levels of detail" )
		MAP_ATTRIBUTE( "lodResource", m_lodResource, "Resource LOD wrapper for texture", Be::PERSISTONLY )
		MAP_PROPERTY( "lodResource", GetLodResource, SetLodResource, "Resource LOD wrapper for texture" )
	EXPOSURE_CHAINTO( TriTextureParameter )
}
