#pragma once

#ifndef EveEffectRoot2_h
#define EveEffectRoot2_h

#include "include/ITriFunction.h"

#include "EveTransform.h"
#include "EveLODHelper.h"
#include "IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Tr2ShLightingManager.h"
#include "Include/ITriTargetable.h"

BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );

BLUE_DECLARE( EveEffectRoot2 );

class EveEffectRoot2:
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2SecondaryLightSource,
	public ITriTargetable
{
public:
    EXPOSE_TO_BLUE();
	using IEveSpaceObject2::Lock;
	using IEveSpaceObject2::Unlock;

	EveEffectRoot2( IRoot* lockobj = NULL );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void UpdateVisibility(  const TriFrustum& frustum, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void GetLights( Tr2LightManager& lightManager ) const;
	void GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const;
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 &position ) const;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	void GetLocalToWorldTransform( Matrix &transform ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2SecondaryLightSource
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& );
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	bool GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );
	void GetImpactPosition( Vector3& out, int damageLocatorIndex, const Vector3& direction );
	bool HasImpactConfigurationShield() const;
	

	void Start();
	void Stop();

private:
	// general
	std::string m_name;
	bool m_display;

	void UpdateWorldTransform( Be::Time time );

	PIEveSpaceObjectChildVector m_effectChildren;


	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;

	Be::Time m_startTime;

	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	
	ITriQuaternionFunctionPtr m_modelRotation;
	ITriVectorFunctionPtr m_modelTranslation;
	
	PTriCurveSetVector m_curveSets;

	Vector4 m_boundingSphere;
	
	// last known results from updating m_ballPosition and m_ballRotation
	Matrix m_worldTransform;
	Matrix m_lastUpdateMatrix;
	Matrix m_localTransform;

	float m_secondaryLightingSphereRadiusLocal;
	float m_secondaryLightingSphereRadiusWorld;
	Color m_secondaryLightingEmissiveColor;

	// PlacementObservers
	PTriObserverLocalVector m_observers;

	// Lods
	bool m_dynamicLODSelection;
	bool m_changeLOD;
	Tr2Lod m_lodLevel;
	
	PTr2PointLightVector m_lights;
	
	float GetBoundingSphereRadius() { return m_boundingSphere.w; }
	float m_estimatedSize;
	float m_effectDuration;
	
	IBlueEventListenerPtr m_loadedEventListener;

};

TYPEDEF_BLUECLASS( EveEffectRoot2 );

#endif // EveEffectRoot2_h