////////////////////////////////////////////////////////////
//
//    Created:   October 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveDistanceField_H
#define EveDistanceField_H

BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_VECTOR( ITriVectorFunction );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( TriView );

// --------------------------------------------------------------------------------------
// Description:
//   Keeps track of a number of objects, max camera distance from them and estimates
//   a simple volume the covers the objects.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveDistanceField ):
	public IListNotify
{
public:
	EveDistanceField( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	void Update( const EveUpdateContext& updateContext );
private:
	PITriVectorFunctionVector m_objects;

	TriCurveSetPtr m_curveSet;

	TriViewPtr m_cameraView;

        // distance value used by curve set
	float m_distance;
	// Adjust how long it takes to settle on a new value when zooming out
	float m_timeAdjustmentSecondsOut;
	// Adjust how long it takes to settle on a new value when zooming in
	float m_timeAdjustmentSecondsIn;

	// used for rejecting objects that are too far away when calcuating
	// area bounds for this field
	float m_distanceThreshold;
	// maximum allowed ratio between x and z sizes of field
	float m_maxXZRatio;
	// minimum allowed ratio between y size and max of x and z sizes
	float m_minYRatio;

	// indicate if bounds must be re-evaluated
	bool m_dirty;

	// middle of the field modified by distance threshold and so on
	Vector3 m_middle;
	// dimensions of the field/volume
	Vector3 m_dimensions;
	
	void SetNeutralValues();
	void CalculateFieldCoverage( Be::Time t );
};
	
TYPEDEF_BLUECLASS( EveDistanceField );

#endif