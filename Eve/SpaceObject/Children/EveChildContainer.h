////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildContainer_H
#define EveChildContainer_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );
BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );

BLUE_CLASS( EveChildContainer ) :
	public IEveSpaceObjectChild,
	public EveChildTransform
{
public:
	EXPOSE_TO_BLUE();

	EveChildContainer( IRoot* lockobj = NULL );
	~EveChildContainer();

	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, Matrix& parentTransform );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;

	void PlayAllCurveSets();
	void StopAllCurveSets();
	
	PIEveSpaceObjectChildVector m_objects;
protected:
	BlueSharedString m_name;
	bool m_display;
	bool m_hideOnLowQuality;
	PTriCurveSetVector m_curveSets;
	PTriObserverLocalVector m_observers;
	PTr2PointLightVector m_lights;
};

TYPEDEF_BLUECLASS( EveChildContainer );

#endif