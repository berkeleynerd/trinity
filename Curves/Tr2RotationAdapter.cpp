////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2RotationAdapter.h"
#include "Vector3d.h"


// --------------------------------------------------------------------------------
Tr2RotationAdapter::Tr2RotationAdapter( IRoot* )
	:m_value( 0, 0, 0, 1 ),
	m_currentValue( 0, 0, 0, 1 ),
	m_start( 0 )
{

}

// --------------------------------------------------------------------------------
void Tr2RotationAdapter::UpdateValue( double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, time );
	}
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::Update( Quaternion* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, TimeAsDouble( time - m_start ) );
	}
	else
	{
		m_currentValue = m_value;
	}
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::Update( Quaternion* in, double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, time );
	}
	else
	{
		m_currentValue = m_value;
	}
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueAt( Quaternion* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		m_curve->GetValueAt( in, TimeAsDouble( time - m_start ) );
	}
	else
	{
		*in = m_value;
	}
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueAt( Quaternion* in, double time )
{
	if( m_curve )
	{
		m_curve->GetValueAt( in, time );
	}
	else
	{
		*in = m_value;
	}
	return in;

}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDotAt( Quaternion* in, Be::Time time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDotAt( Quaternion* in, double time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDoubleDotAt( Quaternion* in, Be::Time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDoubleDotAt( Quaternion* in, double )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}
