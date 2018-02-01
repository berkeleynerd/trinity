////////////////////////////////////////////////////////////
//
//    Created:   February 2018
//    Copyright: CCP 2018
//

#pragma once

#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "ITr2Renderable.h"
#include "Particle/Tr2ParticleElementDeclaration.h"

BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE_INTERFACE( ITr2AttributeGenerator );
BLUE_DECLARE_IVECTOR( ITr2AttributeGenerator );
BLUE_DECLARE( Tr2ParticleSystem );
class EveUpdateContext;

BLUE_CLASS( EveChildParticleSphere ) :
	public IEveSpaceObjectChild,
	public ITr2Renderable
{
public:
	EveChildParticleSphere( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();


	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void ChangeLOD( Tr2Lod lod ) {};
	void PlayCurveSet( const std::string& name ) {};
	void StopCurveSet( const std::string& name ) {};
	void GetLights( Tr2LightManager& lightManager ) const {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	bool HasTransparentBatches();
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	void Refresh();
private:
	void Update( const EveUpdateContext& updateContext );
	void ApplyConstraint( const Vector3& previousReferencePosition, const Vector3& velocityDirection );
	void AddParticles( const Vector3& previousReferencePosition, const Vector3& velocityDirection );

	Tr2InstancedMeshPtr m_mesh;
	Tr2ParticleSystemPtr m_particleSystem;
	PITr2AttributeGeneratorVector m_generators;

	// Particle element data for position
	Tr2ParticleElementData m_positionElement;
	// Particle element data for velocity
	Tr2ParticleElementData m_velocityElement;
	// Particle element data for particle lifetime
	Tr2ParticleElementData m_lifetimeElement;

	Matrix m_worldTransform;
	Vector4 m_boundingSphere;

	std::string m_name;

	enum
	{
		BIND_PENDING,
		BIND_VALID,
		BIND_INVALID,
	} m_bindStatus;

	Vector3 m_previousOrigin;

	// Radius of the effect sphere - particles outise are deleted
	float m_radius;

	// Amount that particles move is scaled by this value
	float m_movementScale;
	// Reference movement is clamped at this speed
	float m_maxSpeed;

	// ego speed
	float m_egoSpeed;

	bool m_useSpaceObjectData;
	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildParticleSphere );