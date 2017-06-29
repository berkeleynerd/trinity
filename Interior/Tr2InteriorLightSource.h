#pragma once
#ifndef Tr2InteriorLightSource_H
#define Tr2InteriorLightSource_H

#include "include/ITr2Interior.h"
#include "TriFrustum.h"
#include "Utilities/BoundingBox.h"

// --------------------------------------------------------------------------------------
// Blue forwards
BLUE_DECLARE( Tr2InteriorLightSource );
BLUE_DECLARE( Tr2KelvinColor );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class ITriRenderBatchAccumulator;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource represents an interior light, which can behave as either a 
//   point or spot light, depending on parameters.  Lights are managed at the 
//   Tr2InteriorScene level.
// See Also:
//   Tr2InteriorScene, Tr2InteriorLightSet
// --------------------------------------------------------------------------------------
class Tr2InteriorLightSource :
	public INotify,
	public IInitialize,
	public ITr2InteriorLight
{
public:
	// Constructor
	Tr2InteriorLightSource( IRoot* lockobj = 0 );
	// Destructor
	~Tr2InteriorLightSource();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	// Initialization callback
    bool Initialize();

	// Value-modified callback
	bool OnModified( Be::Var* val );

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

	/////////////////////////////////////////////////////////////
	// ITr2InteriorLight

	// Copy the light parameters into the per-object data
	void PopulateLightData( Tr2InteriorPerObjectLightData* lightData ) const;

	// Determine overall scene influence
	float GetCurrentViewImportance( const Vector3& viewerPos ) const;

	void Update( Be::Time time );

protected:
	// Is this a spotlight?
	bool IsSpotLight()					const { return ( m_coneAlphaOuter < 89.f ); }
protected:

	// Light names
	std::string m_name;

	// Light position
	Vector3 m_position;
	// Light radius
	float m_radius;
	AxisAlignedBoundingBox m_worldBoundingBox;
	// Light diffuse Color
	Color m_color;
	// Falloff over radius
	float m_falloff;
	// Specular intensity
	float m_specularIntensity;
	// Outer spotlight angle
	float m_coneAlphaOuter;
	// Inner spotlight angle
	float m_coneAlphaInner;
	// Spotlight direction
	Vector3 m_coneDirection;

	// Kelvin color temperature
	Tr2KelvinColorPtr m_kelvinColor;
	// Toggle between rgb and kelvin
	bool m_useKelvinColor;

	// Does this light contribute to direct lighting?
	bool m_primaryLighting;

	// A multiplier for importance. In practice actually for turning off sorting, or by
	// enforcing a certain order, by setting it to zero for all lights and tweaking bias instead.
	float m_importanceScale;
	// A bias. Final view importance is [computed] * scale + bias
	float m_importanceBias;

	// Cached unit to world light geometry transform for light accumulation pass
	Matrix m_unitToWorldTransform;

	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( Tr2InteriorLightSource );
BLUE_DECLARE_VECTOR( Tr2InteriorLightSource );

#endif // Tr2InteriorLightSource_H
