////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"


BLUE_CLASS( Tr2CurveRandomAxisRotation ): public ITriQuaternionFunction
{
public:
	Tr2CurveRandomAxisRotation( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual Quaternion* Update( Quaternion* in, Be::Time time );
	virtual Quaternion* Update( Quaternion* in, double time );
	virtual Quaternion* GetValueAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, double time );

	Quaternion GetValue( double time ) const;
public:
	std::string m_name;

	Quaternion m_preRotation;
	Quaternion m_postRotation;
	Quaternion m_currentValue;
	float m_period;
};

TYPEDEF_BLUECLASS( Tr2CurveRandomAxisRotation );