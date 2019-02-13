#include "StdAfx.h"
#include "Tr2FileExportInfo.h"

BLUE_DEFINE( Tr2FileExportInfo );
BLUE_DEFINE( Tr2FileExportObject );

const Be::ClassInfo* Tr2FileExportObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FileExportObject, "" )
		MAP_INTERFACE( Tr2FileExportObject )
		MAP_ATTRIBUTE( "object", m_object, "The exported object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "info", m_info, "The exported info object", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2FileExportInfo::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FileExportInfo, "" )
		MAP_INTERFACE( Tr2FileExportInfo )
		MAP_ATTRIBUTE( "objects", m_objects, "List of exported objects", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "tool", m_tool, "The name of the exporting tool", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "version", m_version, "The version of the exported tool", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
