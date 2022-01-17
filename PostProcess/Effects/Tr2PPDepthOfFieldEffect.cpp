////////////////////////////////////////////////////////////////////////////////
//
// Created:		December 2021
// Copyright:	CCP 2021
//

#include "StdAfx.h"
#include "Tr2PPDepthOfFieldEffect.h"


Tr2PPDepthOfFieldEffect::Tr2PPDepthOfFieldEffect( IRoot* lockobj ) :
	m_focalDistance( 0.0f ),
	m_focalLength(0.0f),
	m_scale( 0.0f )
{
}

Tr2PPDepthOfFieldEffect::~Tr2PPDepthOfFieldEffect()
{
}

bool Tr2PPDepthOfFieldEffect::IsActive()
{
	return Tr2PPEffect::IsActive() && m_scale > 0.0f;
}
