////////////////////////////////////////////////////////////
//
//    Created:   December 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

BLUE_DEFINE( Tr2DebugRenderer );


const Be::ClassInfo* Tr2DebugRenderer::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2DebugRenderer, "" )
		MAP_INTERFACE( Tr2DebugRenderer )

		MAP_METHOD_AND_WRAP( 
			"SetSelectedObjects", 
			SetSelectedObjects, 
			"Assign a set of selected objects. Object may render differently in selected mode\n"
			":param objects: selected objects\n" 
		)
		MAP_METHOD_AND_WRAP( 
			"SetOptions", 
			SetOptions, 
			"Assign a set of visualization options for a single object\n"
			":param obj: object\n" 
			":param options: set of visualization options\n" 
		)
		MAP_METHOD_AND_WRAP( 
			"SetDefaultOptions", 
			SetDefaultOptions, 
			"Assign a set of fallback visualization options\n"
			":param options: set of visualization options\n" 
		)
	EXPOSURE_END()
}
