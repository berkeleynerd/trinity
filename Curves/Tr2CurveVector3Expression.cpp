#include "StdAfx.h"
#include "Tr2CurveVector3Expression.h"
#include "include/TriMath.h"

extern bool g_expressionCurveFakeRandom;


namespace
{
	CcpMutex s_mutex( "Tr2CurveScalarExpression", "s_mutex", 1000 );

	std::vector<const Tr2CurveVector3Expression*> s_currentCurve;

	// --------------------------------------------------------------------------------
	float Fractal( float x, float alpha, float beta, float n )
	{
		return float( ( PerlinNoise1D( x + s_currentCurve.back()->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float Noise( float x )
	{
		return float( ( PerlinNoise1D( x + s_currentCurve.back()->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float RandomConstant( float a, float b )
	{
		return ( ( b - a ) * s_currentCurve.back()->GetRandomConstant() ) + a;
	}

	// --------------------------------------------------------------------------------
	float Random( float a, float b )
	{
		if( g_expressionCurveFakeRandom )
		{
			return ( ( b - a ) * 0.41f ) + a;
		}
		return ( ( b - a ) * ( (float)rand() / RAND_MAX ) ) + a;
	}

	// --------------------------------------------------------------------------------
	float Input( float index )
	{
		return s_currentCurve.back()->GetInputValue( int32_t( index + 0.5f ) );
	}

	// --------------------------------------------------------------------------------
	float InputAt( float index, float time )
	{
		return s_currentCurve.back()->GetInputValue( int32_t( index + 0.5f ), time );
	}
}

// --------------------------------------------------------------------------------
Tr2CurveVector3Expression::Tr2CurveVector3Expression( IRoot* lockobj )
	:PARENTLOCK( m_inputs ),
	m_currentValue( 0, 0, 0 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX ),
	m_input1( 0 ),
	m_input2( 0 ),
	m_input3( 0 ),
	m_input4( 0 )
{
	for( size_t i = 0; i < 3; ++i )
	{
		auto& parser = m_expressionParsers[i];

		parser.DefineFun( "fractal", &Fractal, false );
		parser.DefineFun( "noise", &Noise, false );
		parser.DefineFun( "randomConstant", &RandomConstant, false );
		parser.DefineFun( "randconst", &RandomConstant, false );
		parser.DefineFun( "random", &Random, false );
		parser.DefineFun( "input", &Input, false );
		parser.DefineFun( "inputAt", &InputAt, false );
		parser.DefineFun( "clamp", &Clamp, false );

		parser.DefineVar( "input1", &m_input1 );
		parser.DefineVar( "input2", &m_input2 );
		parser.DefineVar( "input3", &m_input3 );
		parser.DefineVar( "input4", &m_input4 );

		parser.DefineVar( "time", &m_time );

		parser.DefineConst( "pi", 3.1415926f );
		parser.DefineConst( "pi2", 2.0f * 3.1415926f );
	}
}

// --------------------------------------------------------------------------------
bool Tr2CurveVector3Expression::Initialize()
{
	for( size_t i = 0; i < 3; ++i )
	{
		if( !m_expressions[i].empty() )
		{
			auto expression = m_expressions[i];
			m_expressions[i] = "";
			SetExpression( i, expression );
		}
	}
	return true;

}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Vector3 Tr2CurveVector3Expression::GetValue( double time ) const
{
	Vector3 result( 0, 0, 0 );
	float* components = &result.x;

	m_time = float( time / m_timeScale );

	for( size_t i = 0; i < 3; ++i )
	{
		if( m_expressions[i].empty() )
		{
			continue;
		}

		CcpAutoMutex lock( s_mutex );
		s_currentCurve.push_back( this );

		try
		{
			components[i] = m_expressionParsers[i].Eval();
		}
		catch( const mu::Parser::exception_type& )
		{
			components[i] = 0;
		}
		s_currentCurve.pop_back();
	}
	return result;

}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpression( size_t index ) const
{
	return m_expressions[index];
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpression( size_t index, const std::string& expression )
{
	if( expression.empty() )
	{
		m_expressions[index] = expression;
		return;
	}
	m_expressionParsers[index].SetExpr( expression );

	CcpAutoMutex lock( s_mutex );
	s_currentCurve.push_back( this );

	try
	{
		m_expressionParsers[index].Eval();
	}
	catch( const mu::Parser::exception_type& e )
	{
		s_currentCurve.pop_back();
		CCP_LOGERR( "Tr2CurveVector3Expression::SetExpression invalid expression \"%s\": %s", expression.c_str(), e.GetMsg().c_str() );
		m_expressionParsers[index].SetExpr( m_expressions[index] );
		return;
	}
	s_currentCurve.pop_back();
	m_expressions[index] = expression;
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionX() const
{
	return GetExpression( 0 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionX( const std::string& expression )
{
	SetExpression( 0, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionY() const
{
	return GetExpression( 1 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionY( const std::string& expression )
{
	SetExpression( 1, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionZ() const
{
	return GetExpression( 2 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionZ( const std::string& expression )
{
	SetExpression( 2, expression );
}

// --------------------------------------------------------------------------------
float Tr2CurveVector3Expression::GetRandomConstant() const
{
	if( g_expressionCurveFakeRandom )
	{
		return 0.21f;
	}
	else
	{
		return m_randomConstant;
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveVector3Expression::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_time );
}

// --------------------------------------------------------------------------------
float Tr2CurveVector3Expression::GetInputValue( int index, float time ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( time );
}
