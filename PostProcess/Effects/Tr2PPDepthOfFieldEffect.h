////////////////////////////////////////////////////////////////////////////////
//
// Created:		11/24/2021
// Copyright:	CCP 2021
//

#pragma once

#include "Tr2PPEffect.h"

BLUE_CLASS( Tr2PPDepthOfFieldEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPDepthOfFieldEffect( IRoot* lockobj = NULL );
	~Tr2PPDepthOfFieldEffect();

	// Tr2PPEffect
	bool IsActive() override;

	float m_focalDistance;
	float m_focalLength;
	float m_scale;
};
TYPEDEF_BLUECLASS( Tr2PPDepthOfFieldEffect );
