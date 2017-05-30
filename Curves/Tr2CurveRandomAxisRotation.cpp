////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveRandomAxisRotation.h"

namespace
{
	float RandAngle()
	{
		return float( rand() ) / RAND_MAX * XM_PI * 2;
	}
}

// --------------------------------------------------------------------------------
Tr2CurveRandomAxisRotation::Tr2CurveRandomAxisRotation( IRoot* lockobj )
	:m_preRotation( XMQuaternionRotationRollPitchYaw( RandAngle(), RandAngle(), RandAngle() ) ),
	m_postRotation( XMQuaternionRotationRollPitchYaw( RandAngle(), RandAngle(), RandAngle() ) ),
	m_period( 1.f )
{
	m_currentValue = GetValue( 0 );
}

// --------------------------------------------------------------------------------
void Tr2CurveRandomAxisRotation::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::Update( Quaternion* in, Be::Time time )
{
	auto period = TimeFromDouble( std::abs( m_period ) );
	m_currentValue = GetValue( TimeAsDouble( time % period ) );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::Update( Quaternion* in, double time )
{
	m_currentValue = GetValue( time );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueAt( Quaternion* in, Be::Time time )
{
	auto period = TimeFromDouble( std::abs( m_period ) );
	*in = GetValue( TimeAsDouble( time % period ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueAt( Quaternion* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDoubleDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDoubleDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveRandomAxisRotation::GetValue( double time ) const
{
	Quaternion result = m_postRotation;
	if( m_period != 0 )
	{
		result *= Quaternion( XMQuaternionRotationRollPitchYaw( float( time / double( std::abs( m_period ) ) ) * XM_2PI, 0, 0 ) );
	}
	result *= m_preRotation;
	return result;
}