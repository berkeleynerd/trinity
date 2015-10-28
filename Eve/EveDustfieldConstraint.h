////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef EveDustfieldConstraint_H
#define EveDustfieldConstraint_H

#include "Particle/ITr2GenericParticleConstraint.h"
#include "Particle/Tr2ParticleElementDeclaration.h"

BLUE_DECLARE_INTERFACE( IEveBallpark );
BLUE_DECLARE( Tr2ScalarCurve );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( TriView );

// --------------------------------------------------------------------------------------
// Description:
//   EveDustfieldConstraint is a very specific particle system constraint that is used
//   for EVE dustfield effect. The constaint overrides particle velocities and updates
//   particle age based on the distance to camera.
// See Also:
//   Tr2ParticleSystem, ITr2GenericParticleConstraint
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveDustfieldConstraint ):
	public ITr2GenericParticleConstraint
{
public:
	EveDustfieldConstraint( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////
	// ITr2ParticleConstraint
	virtual void ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particle, unsigned* strides, unsigned count, float dt );
	virtual void Bind( Tr2ParticleSystem* system );

	void Update( const EveUpdateContext& updateContext, IEveBallparkPtr ballpark );
private:
	// Is valid flag (for debugging)
	bool m_isValid;

	// Particle element data for position
	Tr2ParticleElementData m_positionElement;
	// Particle element data for velocity
	Tr2ParticleElementData m_velocityElement;
	// Particle element data for particle lifetime
	Tr2ParticleElementData m_lifetimeElement;

	TriViewPtr m_cameraView;

	// Resulting velocity stretch value
	float m_velocityStretch;
	// Reference (camera) position - used to update particle generators
	Vector3 m_referencePosition;
	// Origin shift compared to previous frame
	Vector3 m_originShift;

	// Current ego ball velocity
	Vector3 m_velocity;
	// Radius of the effect sphere - particles outise are deleted
	float m_radius;
	// Velocity stretch coefficient
	float m_stretch;
	// Maximum velocity stretch value
	float m_maxStretch;

	// Scalar function that's driven by the eye's distance from lookat point
	Tr2ScalarCurvePtr m_distanceFunction;
	// Scalar function that's driven by speed
	Tr2ScalarCurvePtr m_speedFunction;

	// Amount that particles move is scaled by this value
	float m_movementScale;
	// Reference movement is clamped at this speed
	float m_maxSpeed;
};

TYPEDEF_BLUECLASS( EveDustfieldConstraint );

#endif // EveDustfieldConstraint_H