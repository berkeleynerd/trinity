////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveChildLink_H
#define EveChildLink_H

#include "EveChildMesh.h"

// forwards
BLUE_DECLARE_INTERFACE( ITr2ValueBinding );
BLUE_DECLARE_INTERFACE( ITriFunction );
BLUE_DECLARE_IVECTOR( ITriFunction );
BLUE_DECLARE_IVECTOR( ITr2ValueBinding );

BLUE_CLASS( EveChildLink ) :
	public EveChildMesh
{
public:
	EXPOSE_TO_BLUE();

	EveChildLink( IRoot* lockobj = NULL );
	~EveChildLink();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );

private:
	// the size of this thing
	float m_scale;
	// what's it pointing at?
	Vector3 m_direction;
	// how string is the link?
	float m_linkStrength;

	// a link has it's own set of curves for link strength lookup
	PITr2ValueBindingVector m_linkStrengthBindings;
	PITriFunctionVector m_linkStrengthCurves;
};

TYPEDEF_BLUECLASS( EveChildLink );

#endif