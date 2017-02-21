////////////////////////////////////////////////////////////
//
//    Created:   March 2015
//    Copyright: CCP 2015
//

#ifndef _EVELOCALPOSITIONCURVE_H_
#define _EVELOCALPOSITIONCURVE_H_

#include "include/ITriFunction.h"
#include "Eve/IEveSpaceObject2.h"
#include "include/ITriTargetable.h"

BLUE_DECLARE( EveTurretSet );

/* This is an ITriVectorFunction compatible object that calculates a\r\n 
point local to a model. This class has a few behaviours determined by\r\n 
the LocalPositionBehavior enum.\r\n */

class EveLocalPositionCurve:
	public ITriVectorFunction
{

public:
	EXPOSE_TO_BLUE();

	EveLocalPositionCurve( IRoot* lockobj = nullptr );
	~EveLocalPositionCurve();

	enum LocalPositionBehavior
	{
		POS_NONE = 0,
		POS_NEAREST_BOUNDING_POINT,
		POS_CENTER_BOUNDING_POINT,
		POS_TARGET_DMG_LOCATOR,
		POS_TARGET_DMG_LOCATOR_IMPACT,
		POS_OFFSET_POSITION,
		POS_OFFSET_PLANE_ROTATION,
		POS_TURRET_EFFECT_BONE,
		POS_COUNT,
	};
	LocalPositionBehavior m_behavior;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	void UpdateValue( double time ) { Vector3 v; Update( &v, time ); }

	Vector3* Update( Vector3* in, Be::Time time );
	Vector3* Update( Vector3* in, double time );

	Vector3* GetValueAt( Vector3* in, Be::Time time );
	Vector3* GetValueAt( Vector3* in, double time );

	Vector3* GetValueDotAt( Vector3* in, Be::Time time );
	Vector3* GetValueDotAt( Vector3* in, double time );

	Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time );
	Vector3* GetValueDoubleDotAt( Vector3* in, double time );

	Vector3d* InterpolatedPosition( Vector3d* out, Be::Time time );

	// setters
	void SetBehavior( LocalPositionBehavior behavior );

private:
	IEveSpaceObject2Ptr m_parentObject;
	ITriVectorFunctionPtr m_parentPositionCurve;
	ITriQuaternionFunctionPtr m_parentRotationCurve;
	ITriVectorFunctionPtr m_alignPositionCurve;
	EveTurretSetPtr m_turretSet;

	float m_offset;
	Vector3 m_value;
	Vector3 m_boundingBoxSize;
	Vector3 m_positionOffset;

	// damage locator data
	int m_damageLocatorIndex;

	// impact data
	int m_impactEffectIndex;
	float m_impactSize;
	
	Vector3* CalculateOffsetPlaneRotation( Vector3* in, Be::Time t );
	Vector3* CalculateOffsetPosition( Vector3* in, Be::Time t );
	Vector3* CalculateNearestBoundingPoint( Vector3* in, Be::Time t );
	Vector3* GetCenterBoundingSphere( Vector3* in, Be::Time t );
	Vector3* GetDamageLocator( Vector3* in, Be::Time t );
	Vector3* GetDamageLocatorImpact( Vector3* in, Be::Time t );
	Vector3* GetTurretEffectPosition( Vector3* in, Be::Time t);
};

TYPEDEF_BLUECLASS(EveLocalPositionCurve);

#endif 
