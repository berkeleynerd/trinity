////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2TranslationAdapter.h"
#include "Vector3d.h"


// --------------------------------------------------------------------------------
Tr2TranslationAdapter::Tr2TranslationAdapter( IRoot* )
	:m_start( 0 ),
	m_value( 0, 0, 0 ),
	m_currentValue( 0, 0, 0 )
{

}

// --------------------------------------------------------------------------------
void Tr2TranslationAdapter::UpdateValue( double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, time );
	}
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::Update( Vector3* in, Be::Time time )
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
Vector3* Tr2TranslationAdapter::Update( Vector3* in, double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, TimeFromDouble( time ) - m_start );
	}
	else
	{
		m_currentValue = m_value;
	}
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueAt( Vector3* in, Be::Time time )
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
Vector3* Tr2TranslationAdapter::GetValueAt( Vector3* in, double time )
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
Vector3* Tr2TranslationAdapter::GetValueDotAt( Vector3* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		Vector3 v0, v1;
		m_curve->GetValueAt( &v0, TimeAsDouble( time - m_start ) );
		m_curve->GetValueAt( &v1, TimeAsDouble( time - m_start ) - 0.1 );
		*in = Vector3( ( v1 - v0 ) / 0.1f );
	}
	else
	{
		*in = Vector3( 0, 0, 0 );
	}
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDotAt( Vector3* in, double time )
{
	if( m_curve )
	{
		Vector3 v0, v1;
		m_curve->GetValueAt( &v0, time );
		m_curve->GetValueAt( &v1, time - 0.1 );
		*in = Vector3( ( v1 - v0 ) / 0.1f );
	}
	else
	{
		*in = Vector3( 0, 0, 0 );
	}
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDoubleDotAt( Vector3* in, double )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2TranslationAdapter::InterpolatedPosition( Vector3d* out, Be::Time )
{
	out->x = m_currentValue.x;
	out->y = m_currentValue.y;
	out->z = m_currentValue.z;
	return out;
}
