#include "StdAfx.h"
#include "Tr2CurveVector3Expression.h"

BLUE_DEFINE( Tr2CurveVector3Expression );

const Be::ClassInfo* Tr2CurveVector3Expression::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveVector3Expression, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2CurveVector3Expression )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"expressionX",
			m_expressions[0],
			"Curve expression for x component",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionX",
			GetExpressionX,
			SetExpressionX,
			"Curve expression for x component" )
		MAP_ATTRIBUTE(
			"expressionY",
			m_expressions[1],
			"Curve expression for y component",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionY",
			GetExpressionY,
			SetExpressionY,
			"Curve expression for y component" )
		MAP_ATTRIBUTE(
			"expressionZ",
			m_expressions[2],
			"Curve expression for z component",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionZ",
			GetExpressionZ,
			SetExpressionZ,
			"Curve expression for z component" )

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