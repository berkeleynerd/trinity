////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"

BLUE_CLASS( Tr2TranslationAdapter ): public ITriVectorFunction
{
public:
	Tr2TranslationAdapter( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );
	virtual Vector3* Update( Vector3* in, Be::Time time );
	virtual Vector3* Update( Vector3* in, double time );
	virtual Vector3* GetValueAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3* in, double time );
	virtual Vector3* GetValueDotAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3* in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3* in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d* out, Be::Time time );
private:
	Be::Time m_start;
	ITriVectorFunctionPtr m_curve;
	Vector3 m_value;
	Vector3 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2TranslationAdapter );
