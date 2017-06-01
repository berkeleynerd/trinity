////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveScalarExpression.h"

BLUE_DEFINE( Tr2CurveScalarExpression );

const Be::ClassInfo* Tr2CurveScalarExpression::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveScalarExpression, ":jessica-icon: tree/triscalarcurve.png" )
		MAP_INTERFACE( Tr2CurveScalarExpression )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriScalarFunction )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"expression",
			m_expression,
			"Curve expression",
			Be::PERSISTONLY )
		MAP_PROPERTY( 
			"expression", 
			GetExpression, 
			SetExpression, 
			"Curve expression" )
		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )
		MAP_ATTRIBUTE(
			"inputs",
			m_inputs,
			"Scalar curve inputs",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"input1",
			m_input1,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input2",
			m_input2,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input3",
			m_input3,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input4",
			m_input4,
			"Input variable",
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time"
		)

	EXPOSURE_END()
}