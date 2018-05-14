////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include <muParser.h>

BLUE_DECLARE_IVECTOR( ITriScalarFunction );
BLUE_DECLARE( Tr2ExpressionTermInfo );

BLUE_CLASS( Tr2CurveScalarExpression ) : public ITriScalarFunction, public IInitialize
{
public:
	Tr2CurveScalarExpression( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();

	virtual void UpdateValue( double time );
	virtual float Update( Be::Time time );
	virtual float Update( double time );
	virtual float GetValueAt( Be::Time time );
	virtual float GetValueAt( double time );
	virtual void ScaleTime( float s );

	float GetValue( double time ) const;

	std::string GetExpression() const;
	void SetExpression( const std::string& expression );

	float GetRandomConstant() const;
	float GetInputValue( int index ) const;
	float GetInputValue( int index, float time ) const;

	void ResetRandomConstant();
	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
private:
	std::string m_name;
	std::string m_expression;

	mu::Parser m_expressionParser;

	PITriScalarFunctionVector m_inputs;

	float m_currentValue;
	float m_timeScale;
	float m_randomConstant;
	mutable float m_time;

	float m_input1;
	float m_input2;
	float m_input3;
	float m_input4;
};

TYPEDEF_BLUECLASS( Tr2CurveScalarExpression );