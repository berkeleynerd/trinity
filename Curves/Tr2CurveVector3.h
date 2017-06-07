////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include "Tr2CurveScalar.h"


BLUE_CLASS( Tr2CurveVector3 ):
	public ITriCurveLength,
	public ITriFunction
{
public:
	Tr2CurveVector3( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual float Length();

	Vector3 GetValue( double time ) const;

	void AddKey(
		float time,
		Vector3 value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		Be::Optional<Vector3> leftTangent,
		Be::Optional<Vector3> rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );
private:
	std::string m_name;

	PTr2CurveScalar m_x;
	PTr2CurveScalar m_y;
	PTr2CurveScalar m_z;

	Vector3 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2CurveVector3 );