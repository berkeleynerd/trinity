////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "include/IEveBallpark.h"
#include "EveDustfieldConstraint.h"
#include "EveUpdateContext.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Curves/Tr2ScalarCurve.h"
#include <limits>
#include "TriView.h"



// --------------------------------------------------------------------------------------
// Description:
//   EveDustfieldConstraint default constructor
// --------------------------------------------------------------------------------------
EveDustfieldConstraint::EveDustfieldConstraint( IRoot* lockobj )
	:m_isValid( false ),
	m_radius( 512.0f ),
	m_stretch( 0.05f ),
	m_maxStretch( 15.0f ),
	m_movementScale( 1.f ),
	m_maxSpeed( 0.f ),
	m_originShift( 0.f, 0.f, 0.f )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. Binds the constraint with the
//   provided particle system. The constraint needs POSITION, VELOCITY and LIFETIME 
//   elements to be present in the system.
// Arguments:
//   system - Particle system that owns the constraint
// --------------------------------------------------------------------------------------
void EveDustfieldConstraint::Bind( Tr2ParticleSystem* system )
{
	m_isValid = false;

	const Tr2ParticleElementDataMap &declaration = system->GetElementDeclaration();

	Tr2ParticleElementDeclarationName position( Tr2ParticleElementDeclarationName::POSITION );
	auto i = declaration.find( position );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "EveDustfieldConstraint needs POSITION particle element in the system" );
		return;
	}
	else
	{
		m_positionElement = i->second;
	}

	Tr2ParticleElementDeclarationName velocity( Tr2ParticleElementDeclarationName::VELOCITY );
	i = declaration.find( velocity );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "EveDustfieldConstraint needs VELOCITY particle element in the system" );
		return;
	}
	else
	{
		m_velocityElement = i->second;
	}

	Tr2ParticleElementDeclarationName lifetime( Tr2ParticleElementDeclarationName::LIFETIME );
	i = declaration.find( lifetime );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "EveDustfieldConstraint needs LIFETIME particle element in the system" );
		return;
	}
	else
	{
		m_lifetimeElement = i->second;
	}

	m_isValid = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates dustfield values.
// Arguments:
//   time - Current time
//   ballpark - Something called ballpark
// --------------------------------------------------------------------------------------
void EveDustfieldConstraint::Update( const EveUpdateContext& updateContext, IEveBallparkPtr ballpark )
{
	if( !m_cameraView || !ballpark )
	{
		return;
	}

	Be::Time time = updateContext.GetTime();
	float delta = updateContext.GetDeltaT();
	
	m_originShift = updateContext.GetOriginShift();
	if( m_maxSpeed != 0.f && D3DXVec3Length( &m_originShift ) > m_maxSpeed * delta )
	{
		D3DXVec3Normalize( &m_originShift, &m_originShift );
		D3DXVec3Scale( &m_originShift, &m_originShift, m_maxSpeed * delta );
	}
	m_originShift *= m_movementScale;

	m_referencePosition = m_cameraView->GetTransform().GetTranslation();


	Vector3 direction;
	Vector3 velocity;
	ballpark->DeltaVel( &velocity, time );
	float speed = D3DXVec3Length( &velocity );
	D3DXVec3Normalize( &m_velocity, &velocity );

	m_velocityStretch = min( speed * m_stretch, m_maxStretch );
	if( m_speedFunction )
	{
		m_speedFunction->UpdateValue( (double)speed );
	}

	if( m_distanceFunction )
	{
		double distance = (double)D3DXVec3Length( &m_referencePosition );
		m_distanceFunction->UpdateValue( distance );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. Applies the constraint to the
//   particle. Updates particle velocities based on ego ball velocity and lifetime based
//   on distance to camera.
// Arguments:
//   arguments - (unused) update arguments
//   particles - Particle data buffers
//   strides - Sizes of particle data in each of "particles" arrays (in floats).
//   count - Number of particles.
//   dt - (unused) Frame time
// --------------------------------------------------------------------------------------
void EveDustfieldConstraint::ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particles, unsigned* strides, unsigned count, float dt )
{
	if( !m_isValid )
	{
		return;
	}

	Tr2ParticleStreamIterator<XMFLOAT3> position( particles, strides, m_positionElement );
	Tr2ParticleStreamIterator<XMFLOAT3> velocity( particles, strides, m_velocityElement );
	Tr2ParticleStreamIterator<float> lifetime( particles, strides, m_lifetimeElement );

	XMVECTOR originShift = m_originShift;
	XMVECTOR referencePosition = m_referencePosition;
	XMVECTOR constVelocity = m_velocity;
	XMVECTOR oneOverRadius2 = XMVectorReplicate( 1.f / ( m_radius * m_radius ) );

	for( unsigned i = 0; i < count; ++i, ++position, ++velocity, ++lifetime )
	{
		XMVECTOR localPosition = XMVectorAdd( XMLoadFloat3A( position ), originShift );
		XMStoreFloat3A( position, localPosition );

		localPosition = XMVectorSubtract( localPosition, referencePosition );
		XMStoreFloat( 
			lifetime, 
			XMVectorMin( XMVectorMultiply( XMVector3LengthSq( localPosition ), oneOverRadius2 ), XMVectorSplatOne() ) );

		XMStoreFloat3A( velocity, constVelocity );
	}
}
