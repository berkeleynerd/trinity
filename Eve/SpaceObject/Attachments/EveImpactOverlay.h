////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveImpactOverlay_H
#define EveImpactOverlay_H

//#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE_INTERFACE( ITriScalarFunction );
BLUE_DECLARE( Tr2ScalarFader );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuUniqueEmitter );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );


BLUE_CLASS( EveImpactOverlay ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveImpactOverlay( IRoot* lockobj = NULL );
	~EveImpactOverlay();

	enum ImpactConfiguration
	{
		IMPACT_INVALID = 0,
		IMPACT_SHIELD,
		IMPACT_ARMOR,
		IMPACT_HULL,
	};

	enum
	{
		IMPACT_DATA_ROW_0 = 0,
		IMPACT_DATA_ROW_1,
		IMPACT_DATA_ROW_2,
		IMPACT_DATA_ROW_3,
		IMPACT_DATA_ROW_COUNT
	};

	// a row in the data texture
	struct DataRow
	{
		Vector4 v[IMPACT_DATA_ROW_COUNT];
	};

	// shield impacts
	struct ShieldImpactData
	{
		int damageLocatorIndex;
		Vector3 interceptPosition;
		Vector3 direction;
		float lifeTime;
		float timeLeft;
		float size;
	};

	// armor impacts
	struct ArmorImpactData
	{
		int damageLocatorIndex;
		float size;
		bool requestSpawnDebris;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Updates
	void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// Rendering
	void GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	Tr2EffectPtr GetArmorDamageShader( TriBatchType batchType ) const;

	// getters
	int32_t GetDataTextureOffset() const;
	ImpactConfiguration GetImpactConfiguration() const;
	float GetActivationStrength( EveUpdateContext& updateContext ) const;

	// control animation
	void SetToSkinned();
	void ToggleEffect( const std::string& name, bool on );

	// set the damages
	void SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts );

	// control impacts
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );

	// helper for checking activity
	bool HasGeneralActivity() const;
	bool HasShieldActivity() const;
	bool HasArmorActivity() const;

private:
	// helper functions to create the different types of impacts
	int CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	int CreateArmorImpact( int damageLocatorIndex, float size, bool spawnEffects );

	// general data
	BlueSharedString m_name;
	bool m_display;
	ImpactConfiguration m_configuration;
	int m_impactDataNextIdx;
	bool m_debugForceSpawnDebris;

	// non-directional impacts
	float m_overallShieldImpact;

	// all the data used in the data texture
	DataRow m_impactTexelHeader;
	std::vector<DataRow> m_impactTexelData;

	// the data texture block ID
	int32_t m_dataTextureBlockID;
	int32_t m_dataTextureOffset;

	// a map of all impacts going on at the moment
	std::map<int, ShieldImpactData> m_shieldImpactData;
	std::map<int, ArmorImpactData> m_armorImpactData;

	// shield damage
	Tr2MeshBasePtr m_mesh;
	uint32_t m_maxShieldImpacts;
	float m_shieldImpactColorFade;
	float m_shieldImpactParentSize;

	// armor damage
	Tr2EffectPtr m_armorDamageShader;
	size_t m_armorImpactGoalCount;
	float m_armorImpactParentSize;
	Tr2GpuUniqueEmitterPtr m_armorImpactEmitter;

	// hull damage
	float m_hullDamageFactor;
	ITriScalarFunctionPtr m_hullDamageFlickerCurve;

	// extenders
	Tr2ScalarFaderPtr m_shieldHardening;
	Tr2ScalarFaderPtr m_shieldBoosting;
	Tr2ScalarFaderPtr m_armorRepairing;
	Tr2ScalarFaderPtr m_armorHardening;
};

TYPEDEF_BLUECLASS( EveImpactOverlay );

#endif