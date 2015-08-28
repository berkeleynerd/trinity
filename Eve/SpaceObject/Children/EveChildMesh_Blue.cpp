#include "StdAfx.h"
#include "EveChildMesh.h"

BLUE_DEFINE( EveChildMesh );

const Be::ClassInfo* EveChildMesh::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildMesh, "" )
        MAP_INTERFACE( EveChildMesh )
		MAP_INTERFACE( IEveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_worldTransform, "", Be::READ )

    EXPOSURE_END()
}