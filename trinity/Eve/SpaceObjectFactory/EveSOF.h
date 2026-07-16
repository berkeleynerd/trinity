// Copyright © 2013 CCP ehf.

#pragma once
#ifndef EveSOF_H
#define EveSOF_H

#include "EveSOFDataMgr.h"
#include "ITr2Renderable.h"

#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// forwards
BLUE_DECLARE( EveTurretSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2LodResource );
BLUE_DECLARE_INTERFACE( IEveSpaceObjectDecalOwner );
BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveShip2 );
BLUE_DECLARE( EveSOF );
BLUE_DECLARE( EveSOFDNA );
BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE( EveChildContainer );
BLUE_DECLARE_INTERFACE( IEveEffectChildrenOwner );
BLUE_DECLARE_INTERFACE( ITr2LightOwner );
BLUE_DECLARE_INTERFACE( IEveSpaceObjectAttachment );
BLUE_DECLARE_VECTOR( Tr2MeshArea );
BLUE_DECLARE( EveChildInstancedMeshes );

struct EveSOFGeometrySubstitution
{
	std::string authoredPath;
	std::string stagedPath;
};

// A C++-only census of the native object returned by BuildFromDNA.  This is
// deliberately not exposed to Blue: standalone validation hosts use it to
// distinguish a complete native build from the partial object that legacy SOF
// construction can otherwise return while resources are still settling.
struct EveSOFBuildDiagnostics
{
	std::string dna;
	std::string failureReason;
	std::string authoredHullGeometryPath;
	std::string resolvedHullGeometryPath;
	std::string authoredTrailGeometryPath;
	std::string resolvedTrailGeometryPath;
	std::array<uint32_t, TRIBATCHTYPE_COUNT_OF_BATCH_TYPES> expectedAreaCounts{};
	std::array<uint32_t, TRIBATCHTYPE_COUNT_OF_BATCH_TYPES> actualAreaCounts{};
	uint32_t materialAreaCount = 0;
	uint32_t effectResourceCount = 0;
	uint32_t readyEffectResourceCount = 0;
	uint32_t configuredGeometrySubstitutionCount = 0;
	uint32_t usedGeometrySubstitutionCount = 0;
	uint32_t configuredEffectChildSubstitutionCount = 0;
	uint32_t usedEffectChildSubstitutionCount = 0;
	uint32_t geometryBoneBindingCount = 0;
	uint32_t geometrySkeletonCount = 0;
	uint32_t geometryAnimationCount = 0;
	uint32_t geometrySkinnedAreaCount = 0;
	uint32_t directHullLightCount = 0;
	uint32_t attachmentLightCount = 0;
	uint32_t localLightCount = 0;
	uint32_t boosterCount = 0;
	uint32_t trailCount = 0;
	uint32_t expectedControllerCount = 0;
	uint32_t installedControllerCount = 0;
	int32_t buildClass = -1;
	int32_t impactEffectType = -1;
	int32_t reflectionMode = -1;
	bool callerDataInstalled = false;
	bool controllerClosureComplete = false;
	bool effectChildClosureComplete = false;
	bool strictGeometrySubstitutions = false;
	bool dnaValid = false;
	bool impactEffectNone = false;
	bool objectCreated = false;
	bool objectMatchesLastBuild = false;
	bool nativeShip = false;
	bool meshCreated = false;
	bool meshAreasComplete = false;
	bool materialsComplete = false;
	bool geometryClosureComplete = false;
	bool geometryResourceBound = false;
	bool geometryLoading = false;
	bool geometryPrepared = false;
	bool geometrySkinningComplete = false;
	bool boostersExpected = false;
	bool boostersCreated = false;
	bool trailsExpected = false;
	bool trailsCreated = false;
	bool trailsReady = false;
	bool castsShadow = false;
	bool initialized = false;
	bool structuralComplete = false;
	bool resourceReady = false;
	bool buildSucceeded = false;
};

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOF ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSOF( IRoot* lockobj = NULL );
	~EveSOF();

	// build a spaceship and return a EveShip2 object
	IRootPtr Build( const char* hullName, const char* factionName, const char* raceName );
	// build a spaceship from a dns string and return a EveShip2 object
	IRootPtr BuildFromDNA( const char* dnaString );

	// validate a dna string (slow!)
	bool ValidateDNA( const char* dnaString );

	// change the material of a turret with SOF data
	void SetupTurretMaterialFromDNA( EveTurretSet * turretSet, const char* dnaString );
	void SetupTurretMaterialFromFaction( EveTurretSet * turretSet, const char* factionName );

	bool LoadData( const char* filePath );
	// Install an already-loaded SOF database.  This is the non-yielding seam
	// used by standalone hosts that load data.black through BlackReader.
	bool SetData( EveSOFData* data );

	// Configure an exact authored-GR2 to staged-CMF closure.  Once configured,
	// every GR2 requested by native construction must have a unique mapping;
	// missing or conflicting mappings make BuildFromDNA fail closed.  The
	// authored trail path is supplied separately because production obtains the
	// resolved trail path from a setting rather than from SOF data.
	bool ConfigureGeometrySubstitutions(
		const std::vector<EveSOFGeometrySubstitution>& substitutions,
		const char* authoredVolumetricTrailPath );
	// Install a controller that the authored graph names by resource path.
	// Standalone hosts use this after reading the Black synchronously so native
	// construction never enters Blue's tasklet-yielding LoadObject path.
	bool ConfigureControllerSubstitution( const char* authoredPath, ITr2Controller* controller );
	// Install a child graph named by SOF hull data. Standalone hosts use this
	// beside the controller seam so native construction remains non-yielding.
	bool ConfigureEffectChildSubstitution( const char* authoredPath, IRoot* child );
	void ClearGeometrySubstitutions();
	bool InspectNativeBuild( IRoot* object, EveSOFBuildDiagnostics& diagnostics ) const;
	const EveSOFBuildDiagnostics& GetLastBuildDiagnostics() const;

private:
	// creation
	EveSpaceObject2Ptr CreateSpaceObject( const EveSOFDNAPtr dna ) const;

	struct InheritableTextureKey
	{
		size_t hullIndex;
		int32_t meshIndex;
		BlueSharedString name;

		bool operator==( const InheritableTextureKey& other ) const
		{
			return hullIndex == other.hullIndex && meshIndex == other.meshIndex && name == other.name;
		}

		operator size_t() const
		{
			return hullIndex | ( meshIndex << 4 ) | reinterpret_cast<size_t>( name.c_str() );
		}

		// Use the object as its own hasher (needs to be explicit for non fundamental types in >VS2015)
		size_t operator()( const InheritableTextureKey& key ) const
		{
			return key;
		}
	};

	EveSOFDNAPtr CreateDna( const char* dnaString );

	// all setup functions for the to-be-created space object
	void SetupConsts( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	bool SetupMesh( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupAttachments( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpriteSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpotlightSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupPlaneSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpriteLineSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupHazeSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupBanners( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupBannerSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupEffects( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupChildrenAndAnimations( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupEffectChildren( EveSpaceObject2Ptr newObj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupControllers( ITr2ControllerOwnerPtr newObj, const EveSOFDNAPtr dna, uint32_t buildFlags ) const;
	void SetupAudio( ITr2SoundEmitterOwnerPtr newObj, const EveSOFDNAPtr dna, const Matrix& offset = IdentityMatrix() ) const;
	void SetupInstancedMeshes( EveSpaceObject2Ptr newObj, EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupDecalSets( IEveSpaceObjectDecalOwnerPtr obj, const EveSOFDNAPtr dna ) const;
	void SetupModelCurves( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLocators( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLocatorSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets );
	void SetupImpactEffects( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLights( ITr2LightOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupLayout( EveSpaceObject2Ptr obj, EveChildContainerPtr layoutContainer, EveChildInstancedMeshesPtr & sharedMeshes, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t seedOverwrite = 0 );


	Tr2MeshPtr CreateMesh( const EveSOFDNAPtr dna ) const;
	Tr2InstancedMeshPtr CreateInstancedMesh( std::vector<EveSOFDataMgr::HullMeshInstance> instances, std::string resPath ) const;
	void SetupShaders( const EveSOFDNAPtr dna, Tr2MeshBase* mesh ) const;

	void CreatePlacement(
		EveSpaceObject2Ptr parent,
		EveChildInstancedMeshesPtr & sharedMeshes,
		EveSOFDNAPtr extensionDna,
		const EveSOFDNAPtr& parentDna,
		EveSOFDataMgr::ExtensionPlacementData& placement,
		const std::vector<EveSOFDataMgr::LocatorDirectionData>& locators,
		const std::vector<Matrix>& nestedOffsets,
		EveChildContainerPtr layoutContainer );

	void SetupCustomMask( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;

	// all setup functions for ships only
	bool SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;

	Tr2EffectPtr CreateBoosterEffect( const EveSOFDataMgr::RaceBoosterData* rdata, const BlueSharedString& lodOption ) const;

	bool ProcessLayoutDistributionConditions( EveSOFDataMgr::ExtensionPlacementData & placement, const EveSOFDNAPtr dna );
	void ProcessLayoutDistributionDistribute( EveSOFDataMgr::ExtensionPlacementDistribution & distributionData, const EveSOFDNAPtr dna, std::vector<EveSOFDataMgr::LocatorDirectionData>& placementSet, std::vector<EveSOFDataMgr::LocatorDirectionData>& managedLocatorSet );
	void ProcessPlacementDistributionOrGroup( EveSOFDataMgr::ExtensionPlacementData & distributionData, EveSpaceObject2Ptr obj, EveChildInstancedMeshesPtr & sharedMeshes, const EveSOFDNAPtr dna, std::map<BlueSharedString, std::vector<EveSOFDataMgr::LocatorDirectionData>>& managedLocatorSet, size_t& layoutIdx, size_t& placementIdx, const std::vector<Matrix>& offsets, EveChildContainerPtr childContainer );

	// helper functions
	size_t FillMeshAreaVector( Tr2MeshAreaVector * meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna, size_t hullIdx, size_t meshIndexOffset ) const;
	bool GenerateLodResourcePaths( std::string & mediumResPath, std::string & lowResPath, std::string & ultraResPath, const char* resPath, const char* usage ) const;
	void GenerateDepthFromAreaVector( Tr2MeshBase * mesh, const Tr2MeshAreaVector* meshAreaVector, const EveSOFDNAPtr dna ) const;
	void CreatePointLightData( const Vector3& pos, const float scale, const Color& color, const EveSOFDataMgr::PointLightAttachment* lightData ) const;
	void CreateTexturedPointLightData( const Vector3& pos, const float scale, const std::string& texturePath, const EveSOFDataMgr::PointLightAttachment* lightData ) const;
	bool ResolveGeometryPath( const std::string& authoredPath, std::string& resolvedPath ) const;
	void SetBuildFailure( const std::string& reason ) const;
	void RecordExpectedMeshInventory( const EveSOFDNAPtr dna ) const;
	void RecordActualMeshInventory( const Tr2MeshBase* mesh, EveSOFBuildDiagnostics& diagnostics ) const;

	// all the source data
	PEveSOFDataMgr m_dataMgr;

	// shared
	Tr2EffectPtr m_hazeSetEffectSpherical, m_skinnedHazeSetEffectSpherical, m_hazeSetEffectHalfSpherical;
	Tr2EffectPtr m_spriteSetEffect;
	BlueSharedString m_depthOnlyEffectName, m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_MAX];

	mutable std::unordered_map<std::string, bool> m_existingFilesCache;
	bool m_allowFileCaching;

	bool m_editorMode;

	std::unordered_map<std::string, std::string> m_geometrySubstitutions;
	mutable std::unordered_set<std::string> m_usedGeometrySubstitutions;
	std::string m_authoredVolumetricTrailPath;
	std::unordered_map<std::string, ITr2ControllerPtr> m_controllerSubstitutions;
	mutable std::unordered_set<std::string> m_usedControllerSubstitutions;
	std::unordered_map<std::string, IRootPtr> m_effectChildSubstitutions;
	mutable std::unordered_set<std::string> m_usedEffectChildSubstitutions;
	bool m_requireGeometrySubstitutions;
	bool m_callerDataInstalled;
	mutable EveSOFBuildDiagnostics m_lastBuildDiagnostics;
	mutable const IRoot* m_lastBuiltObjectIdentity;
};

TYPEDEF_BLUECLASS( EveSOF );

#endif // EveSOF_H
