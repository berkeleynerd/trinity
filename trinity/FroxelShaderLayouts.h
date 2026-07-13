// Copyright (c) 2026 CCP ehf.

#pragma once

#include <cstddef>

namespace TrinityFroxelShaderLayouts
{

struct PerFrameData
{
	Vector3 FogColor;
	float BackgroundVisibility;

	float BaseDensity;
	float MaxDistance;
	float MaxDistanceVisibility;
	float EnvironmentIntensity;

	float EnvironmentG;
	float _pad0;
	float _pad1;
	float _pad2;

	CcpMath::Sphere planets[2];
};

struct PerObjectData
{
	Matrix ProjectionMatrix;
	Matrix InverseProjectionMatrix;

	uint32_t ResolutionX;
	uint32_t ResolutionY;
	uint32_t ResolutionZ;
	uint32_t NumDynamicLights;

	Vector3 Jitter;
	float Far;

	Vector3 Scattering;
	float BaseDensity;

	float MaxDistanceVisibility;
	float LightG;
	float EnvironmentIntensity;
	float InverseShadowMapAtlasSize;

	Vector3 Extinction;
	uint32_t ShadowMapAtlasEntryMinSizeLog2;

	Matrix InverseViewMatrix;

	float GodRayNoiseFrequency;
	float GodRayNoiseLerp;
	float GodRayNoiseAnimation;
	float GodRayNoiseIntensity;

	Matrix GodRayNoiseMatrix;

	Vector3 FogNoiseOffset;
	float FogNoiseFrequency;

	float FogNoiseLerp;
	float FogNoiseIntensity;
	Vector2 LinearizeDepthParams;

	Vector4 UnprojectParams;
	Vector4 PreviousProjectParams;
	Matrix ReprojectionMatrix;

	Vector3 SunViewDirection;
	float SunAngle;

	Vector3 SunWorldDirection;
	float pad0;

	Vector3 SunColor;
	float LightProfileTextureWidth;

	Vector4 ShadowMapValues[4];
	Matrix ShadowMatrix[16];
	Vector4 SplitInfo;

	Tr2LightManager::PerLightData DynamicLights[16];

	CcpMath::Sphere planets[2];
};

static_assert( sizeof( PerFrameData ) == 80, "Froxel per-frame shader layout changed" );
static_assert( offsetof( PerFrameData, planets ) == 48, "Froxel per-frame planet offset changed" );

// These values come from the AIR metadata embedded in the current Metal
// calculate/filter/raymarch containers. Keep them explicit so a checkout or
// client-container drift fails at compile time and in the device-free audit.
static_assert( sizeof( PerObjectData ) == 2432, "Froxel per-object shader layout changed" );
static_assert( offsetof( PerObjectData, ProjectionMatrix ) == 0, "ProjectionMatrix offset changed" );
static_assert( offsetof( PerObjectData, InverseProjectionMatrix ) == 64, "InverseProjectionMatrix offset changed" );
static_assert( offsetof( PerObjectData, ResolutionX ) == 128, "Resolution offset changed" );
static_assert( offsetof( PerObjectData, NumDynamicLights ) == 140, "NumDynamicLights offset changed" );
static_assert( offsetof( PerObjectData, Jitter ) == 144, "Jitter offset changed" );
static_assert( offsetof( PerObjectData, InverseViewMatrix ) == 208, "InverseViewMatrix offset changed" );
static_assert( offsetof( PerObjectData, GodRayNoiseMatrix ) == 288, "GodRayNoiseMatrix offset changed" );
static_assert( offsetof( PerObjectData, UnprojectParams ) == 384, "UnprojectParams offset changed" );
static_assert( offsetof( PerObjectData, ReprojectionMatrix ) == 416, "ReprojectionMatrix offset changed" );
static_assert( offsetof( PerObjectData, SunViewDirection ) == 480, "SunViewDirection offset changed" );
static_assert( offsetof( PerObjectData, ShadowMapValues ) == 528, "ShadowMapValues offset changed" );
static_assert( offsetof( PerObjectData, ShadowMatrix ) == 592, "ShadowMatrix offset changed" );
static_assert( offsetof( PerObjectData, SplitInfo ) == 1616, "SplitInfo offset changed" );
static_assert( offsetof( PerObjectData, DynamicLights ) == 1632, "DynamicLights offset changed" );
static_assert( offsetof( PerObjectData, planets ) == 2400, "Planets offset changed" );

} // namespace TrinityFroxelShaderLayouts
