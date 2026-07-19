// Copyright (c) 2026 CCP Games

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined( __GNUC__ )
#define TRINITY_COMBAT_API __attribute__( ( visibility( "default" ) ) )
#else
#define TRINITY_COMBAT_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TrinityCombatTargetProjection
{
	double centerX;
	double centerY;
	double radius;
	double depth;
	double centerDistance;
	double surfaceDistance;
	bool visible;
} TrinityCombatTargetProjection;

typedef struct TrinityCombatSceneDiagnostics
{
	const char* asteroClass;
	const char* asteroDna;
	const char* ventureClass;
	const char* ventureDna;
	int64_t asteroBallId;
	int64_t ventureBallId;
	int32_t asteroMode;
	int32_t ventureMode;
	uint32_t asteroFlags;
	uint32_t ventureFlags;
	uint32_t asteroMeshAreas;
	uint32_t ventureMeshAreas;
	uint64_t submittedOpaqueBatches;
	uint64_t submittedAdditiveBatches;
	uint64_t submittedTransparentBatches;
	double cameraFovDegrees;
	double cameraEye[3];
	double cameraTarget[3];
	double targetOffset[3];
	double targetCenterDistance;
	bool asteroPositionCurveBound;
	bool asteroRotationCurveBound;
	bool venturePositionCurveBound;
	bool ventureRotationCurveBound;
	bool asteroUniqueSceneMembership;
	bool ventureUniqueSceneMembership;
	bool starfieldReady;
	bool nebulaReady;
	bool clientSsaoSelected;
	bool highRasterShadowsSelected;
	bool highTaaSelected;
	bool dynamicReflectionSelected;
	bool silkEnabled;
	bool froxelsEnabled;
} TrinityCombatSceneDiagnostics;

typedef struct TrinityCombatWeaponDiagnostics
{
	const char* launcherResource;
	const char* missileResource;
	const char* impactResource;
	int32_t launcherTypeId;
	int32_t ammunitionTypeId;
	int64_t missileBallId;
	int64_t ownerBallId;
	int64_t targetBallId;
	int32_t missileMode;
	int64_t launchTime;
	int64_t firstCollisionTime;
	double missilePosition[3];
	double missileVelocity[3];
	double renderedWarheadPosition[3];
	double renderedWarheadTargetDistance;
	double renderedWarheadLocatorDistance;
	double missileCollisionRadius;
	double sourceMissileSpeed;
	double simulationMissileSpeed;
	double visualizationSpeedScale;
	double authoredPathOffsetNoiseScale;
	double appliedPathOffsetNoiseScale;
	double pathOffsetNoiseSpeed;
	double visualizationPathOffsetScale;
	double visualImpactPosition[3];
	double visualImpactTargetDistance;
	double maximumExplosionDistance;
	uint32_t launcherCount;
	uint32_t warheadCount;
	uint32_t launcherLocatorCount;
	uint32_t launcherState;
	uint32_t warheadState;
	bool launcherAuthored;
	bool launcherGeometryReady;
	bool launcherCurvesBound;
	bool missileAuthored;
	bool missileCurvesBound;
	bool missileSceneMember;
	bool missileActive;
	bool missileInitialStraightFlight;
	bool missileCollided;
	bool missileExpired;
	bool missileRemoved;
	bool impactAuthored;
	bool authoredVisualContact;
	bool impactActive;
	bool impactSceneMember;
} TrinityCombatWeaponDiagnostics;

TRINITY_COMBAT_API bool TrinityCombatStartup(
	int argc,
	const char* const* argv,
	const char* executableDirectory );
TRINITY_COMBAT_API void* TrinityCombatCreateDevice(
	void* windowHandle,
	uint32_t renderWidth,
	uint32_t renderHeight );
TRINITY_COMBAT_API void TrinityCombatDestroyDevice( void* combat );

// A null packet creates the fixed two-ball rehearsal. A non-null packet must
// be a snapshot previously returned by this API and becomes the replay state.
TRINITY_COMBAT_API bool TrinityCombatConfigureRehearsal(
	void* combat,
	const char* canonicalManifestPath,
	const void* fullStatePacket,
	size_t fullStatePacketSize );
TRINITY_COMBAT_API bool TrinityCombatMeasureInitialSnapshot(
	void* combat,
	size_t* bytesRequired );
TRINITY_COMBAT_API bool TrinityCombatCopyInitialSnapshot(
	void* combat,
	void* destination,
	size_t destinationSize,
	size_t* bytesWritten );
TRINITY_COMBAT_API bool TrinityCombatRenderFrame(
	void* combat,
	int64_t realTime,
	int64_t simulationTime,
	bool captureColor );
TRINITY_COMBAT_API bool TrinityCombatGetCapturedFrame(
	void* combat,
	const uint8_t** pixels,
	uint32_t* width,
	uint32_t* height,
	uint32_t* pitch );
TRINITY_COMBAT_API bool TrinityCombatGetSceneDiagnostics(
	void* combat,
	TrinityCombatSceneDiagnostics* diagnostics );
TRINITY_COMBAT_API bool TrinityCombatGetTargetProjection(
	void* combat,
	TrinityCombatTargetProjection* projection );
TRINITY_COMBAT_API bool TrinityCombatFireMissile(
	void* combat,
	int64_t missileBallId );
TRINITY_COMBAT_API bool TrinityCombatGetWeaponDiagnostics(
	void* combat,
	TrinityCombatWeaponDiagnostics* diagnostics );

#ifdef __cplusplus
}
#endif

#undef TRINITY_COMBAT_API
