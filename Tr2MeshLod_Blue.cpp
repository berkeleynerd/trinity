////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshLod.h"
#include "Resources/TriGeometryRes.h"

BLUE_DEFINE( Tr2MeshLod );

const Be::ClassInfo* Tr2MeshLod::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshLod, "A mesh with levels of detail\n:jessica-deprecated:" )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE
		(
			"geometryRes",
			m_geometryRes,
			"Geometry LOD resource for this mesh",
			Be::PERSISTONLY
		)

		MAP_PROPERTY(
			"geometryRes",
			GetGeometryLodResource,
			SetGeometryResource,
			"Geometry LOD resource for this mesh" )

		MAP_PROPERTY_READONLY
		(
			"geometry",
			GetGeometryResource,
			"TriGeometryRes used as the currently selected level of detail for this mesh"
		)
		
		MAP_ATTRIBUTE
		(
			"associatedResources", 
			m_associatedResources, 
			"List of resources associated with this mesh that can select level of detail", 
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"selectedLod",
			m_selectedLod,
			"Currently selected level of detail",
			Be::READ
		)

		MAP_METHOD_AND_WRAP
		(
			"SelectLod",
			SelectLod,
			"Selects the level of detail on all resources associated with this mesh\n"
			":param level: LOD level"
		)

	EXPOSURE_CHAINTO( Tr2MeshBase )
}