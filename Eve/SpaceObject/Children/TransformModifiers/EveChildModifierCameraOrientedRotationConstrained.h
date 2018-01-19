////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#pragma once
#ifndef EveChildModifierCameraOrientedRotationConstrained_H
#define EveChildModifierCameraOrientedRotationConstrained_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierCameraOrientedRotationConstrained ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierCameraOrientedRotationConstrained( IRoot* lockobj = NULL );
	~EveChildModifierCameraOrientedRotationConstrained();

	Matrix ApplyTransform( const Matrix& transform ) const;
};

TYPEDEF_BLUECLASS( EveChildModifierCameraOrientedRotationConstrained );

#endif