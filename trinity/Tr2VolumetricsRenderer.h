// Copyright © 2023 CCP ehf.

#pragma once

#include "ITr2VolumetricRenderable.h"
#include "TriFrustumOrtho.h"
#include "Tr2ShadowMap.h"
#include "Tr2LightManager.h"
#include "FroxelShaderLayouts.h"
#include "Eve/EveUpdateContext.h"
#include "PostProcess/Tr2PostProcessEnums.h"
#include "Raytracing/Tr2RaytracingGeometry.h"
#include "PriorityBlend.h"

struct Vector3d;

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( EveComponentRegistry );
class TriFrustum;
class ITriRenderBatchAccumulator;


BLUE_INTERFACE( ITr2FroxelFogSettings ) :
	public IRoot
{
public:
	struct FroxelFogSettings
	{
		PostProcessEnums::Priority priority = PostProcessEnums::MEDIUM_PRIORITY;
		float intensity = 1.0;

		PriorityBlend::Attribute<float> thickness = 0.0f;

		PriorityBlend::Attribute<float> lightDirectionality = 0.0f;

		PriorityBlend::Attribute<float> environmentIntensity = 0.0f;
		PriorityBlend::Attribute<float> environmentDirectionality = 0.0f;

		PriorityBlend::Attribute<Color> fogColor = Color( 0.0f, 0.0f, 0.0f, 0.0f );
		PriorityBlend::Attribute<float> backgroundVisibility = 0.0f;


		PriorityBlend::Attribute<float> godRayNoiseIntensity = 0.0f;
		PriorityBlend::Attribute<float> godRayNoiseFrequency = 0.0f;
		PriorityBlend::Attribute<float> godRayNoiseAnimationSpeed = 0.0f;

		PriorityBlend::Attribute<float> fogNoiseIntensity = 0.0f;
		PriorityBlend::Attribute<float> fogNoiseFrequency = 0.0f;
		PriorityBlend::Attribute<Vector3> fogNoiseMovementSpeed = Vector3( 0.0f, 0.0f, 0.0f );

		PriorityBlend::Attribute<double> logThickness = 0.0;
	};
	virtual FroxelFogSettings* GetFroxelFogSettings() = 0;
};

REGISTER_COMPONENT_TYPE( "FroxelFogSettings", ITr2FroxelFogSettings );


BLUE_CLASS( Tr2VolumetricsRenderer ) :
	public IRoot
{
public:
	Tr2VolumetricsRenderer( IRoot* lockobj = nullptr );

	Tr2GpuResourcePool::Texture RenderVolumetrics(
		const EveComponentRegistry& registry,
		const TriFrustum& frustum,
		const Tr2TextureAL& sceneDepth,
		const Tr2TextureAL& froxelFog,
		const Vector3& sunDirection,
		const float depthSlices[4],
		bool raytracingEnabled,
		Tr2GpuResourcePool& gpuResourcePool,
		Tr2RenderContext& renderContext );
	static Tr2GpuResourcePool::Texture GetEmptyVolumetricTexture( Tr2GpuResourcePool & gpuResourcePool );

	void UpdateFogSettings( const EveComponentRegistry& registry, const EveUpdateContext& updateContext );
	bool HasFog() const;

	Tr2GpuResourcePool::Texture RenderFog(
		Tr2RenderContext & renderContext,
		Tr2GpuResourcePool & gpuResourcePool,
		uint32_t width,
		uint32_t height,
		Tr2ShadowMap* cascadedShadowMap,
		Tr2RaytracingGeometryPtr raytracingGeometry,
		ShadowQuality shadowQuality,
		const Vector3& sunDirection,
		const Color& sunColor,
		const Vector3d origin,
		const Vector3d originShift,
		const Matrix& view,
		const Matrix& projection,
		const Matrix& viewLast,
		const Matrix& projectionLast );
	Tr2GpuResourcePool::Texture RenderFogIntoReflectionMap(
		Tr2RenderContext & renderContext,
		Tr2GpuResourcePool & gpuResourcePool,
		uint32_t width,
		uint32_t height,
		const Vector3& sunDirection,
		const Color& sunColor,
		const Vector3d origin,
		const Matrix& view,
		const Matrix& projection );
	static Tr2GpuResourcePool::Texture GetEmptyFogTexture( Tr2GpuResourcePool & gpuResourcePool );
	void UpdateFogEnvironmentMap( Tr2RenderContext & renderContext );

	void UpdateVariableStore();
	void SetPlanets( const CcpMath::Sphere* planets, size_t planetCount );
	void SetSunAngle( float angle );
	void RenderShadows(
		const EveComponentRegistry& registry,
		const Tr2TextureAL& shadowMap,
		Tr2RenderContext& renderContext );


	using FroxelPerFrameData = TrinityFroxelShaderLayouts::PerFrameData;

	void PopulatePerFrameData( FroxelPerFrameData & data );

	void SetQuality( Tr2VolumerticQuality quality );
	Tr2VolumerticQuality GetQuality() const
	{
		return m_quality;
	}

	struct Diagnostics
	{
		bool froxelEnabled = false;
		bool temporalEnabled = false;
		bool calculateSucceeded = false;
		bool temporalFilterSucceeded = false;
		bool raymarchSucceeded = false;
		bool applySucceeded = false;
		bool mieUpdateSucceeded = false;
		bool localDepthDownsampleSucceeded = false;
		bool localBlurSucceeded = false;
		bool localBlitSucceeded = false;
		bool localLayerPackSucceeded = false;
		bool localOutputCopySucceeded = false;
		uint32_t localRenderableCount = 0;
		uint32_t localBatchCount = 0;
		uint32_t localLightmapUpdates = 0;
		uint32_t localShadowBatchCount = 0;
		uint64_t totalLocalLightmapUpdates = 0;
		uint64_t totalLocalShadowBatches = 0;
		uint32_t volumeWidth = 0;
		uint32_t volumeHeight = 0;
		uint32_t volumeLayers = 0;
		uint32_t volumeFormat = 0;
		uint32_t froxelWidth = 0;
		uint32_t froxelHeight = 0;
		uint32_t froxelDepth = 0;
		uint32_t froxelFormat = 0;
		uint32_t mieWidth = 0;
		uint32_t mieHeight = 0;
		uint32_t mieFormat = 0;
		uint32_t dynamicLightCount = 0;
		uint32_t raymarchThreadGroupX = 0;
		uint32_t raymarchThreadGroupY = 0;
		uint32_t raymarchThreadGroupZ = 0;
		uint32_t raymarchDispatchX = 0;
		uint32_t raymarchDispatchY = 0;
		uint32_t raymarchDispatchZ = 0;
	};

	const Diagnostics& GetDiagnostics() const
	{
		return m_lastDiagnostics;
	}
	void SetLocalOutputCopySucceeded( bool succeeded )
	{
		m_lastDiagnostics.localOutputCopySucceeded = succeeded;
	}
	bool SetNoiseSeed( uint32_t seed, Tr2RenderContext& renderContext );
	void ResetTemporalHistory();
	const Tr2TextureAL* GetMieEnvironmentMap() const;

	EXPOSE_TO_BLUE();


	Tr2RaytracingPipelineStateManager m_pipelineManager;
	Tr2RtShaderTableDescriptionAL m_shaderTableDesc;

private:
	struct FogViewDependentResources
	{
		explicit FogViewDependentResources( bool temporalFroxels );

		Tr2EffectPtr calculateFroxels;
		Tr2EffectPtr rtCalculateFroxels;
		Tr2EffectPtr filterFroxels;
		Tr2EffectPtr raymarchFroxels;
		Tr2EffectPtr applyFroxels;

		Vector3 froxelJitter;
		bool useTemporalFroxels;
		bool currentTemporalFroxels;
		bool resetTemporalFroxels = true;
	};


	Tr2GpuResourcePool::Texture RenderFog(
		FogViewDependentResources & resources,
		Tr2RenderContext & renderContext,
		Tr2GpuResourcePool & gpuResourcePool,
		uint32_t originalWidth,
		uint32_t originalHeight,
		Tr2ShadowMap* cascadedShadowMap,
		Tr2RaytracingGeometryPtr raytracingGeometry,
		ShadowQuality shadowQuality,
		const Vector3& sunDirection,
		const Color& sunColor,
		const Vector3d origin,
		const Vector3d originShift,
		const Matrix& view,
		const Matrix& projection,
		const Matrix& viewLast,
		const Matrix& projectionLast );

	Tr2EffectPtr m_volumeBlit;
	Tr2EffectPtr m_downsampleDepth;
	Tr2EffectPtr m_hBlur;
	Tr2EffectPtr m_vBlur;

	uint32_t m_lastRequestedWidth = 0;
	uint32_t m_lastRequestedHeight = 0;




	Tr2TextureReferencePtr m_mieEnvironmentMap;
	float m_environmentRandom;
	Vector2 m_environmentJitter;
	float m_previousEnvironmentG;
	float m_environmentBlendCounter;

	bool m_logBlending;
	double m_logBlendingSmoothness;
	ITr2FroxelFogSettings::FroxelFogSettings m_froxelFogSettings;

	float m_gameBackClip;

	Tr2TextureReferencePtr m_froxel3DNoise;
	uint32_t m_noiseSeed = 0;


	double m_godRayNoiseAnimation;
	Vector3d m_fogNoiseMovement;

	float m_testValue;
	double m_godRayNoiseMatrix[16];

	FogViewDependentResources m_fogResources;
	FogViewDependentResources m_fogReflectionResources;

	Tr2EffectPtr m_updateMieEnvironmentMap;
	Diagnostics m_lastDiagnostics;
	uint64_t m_totalLocalLightmapUpdates = 0;
	uint64_t m_totalLocalShadowBatches = 0;

	using FogPerObjectData = TrinityFroxelShaderLayouts::PerObjectData;

	void UpdatePerObjectData( FogPerObjectData * data, const Matrix& view, const Matrix& projection, const Matrix& viewLast, const Matrix& projectionLast, const Vector3d& origin, const Vector3d& originShift, const Vector3& sunDirection, const Color& sunColor, uint32_t width, uint32_t height, uint32_t depth, const Vector3& jitter, const Tr2ShadowMap* cascadedShadowMap );

	Tr2ConstantBufferAL m_fogConstantBuffer;


	std::unique_ptr<ITriRenderBatchAccumulator> m_batches;

	Tr2ConstantBufferAL m_shadowPerFrameVSBuffer;
	Tr2VolumerticQuality m_quality;
	float m_scaleFactor;
	bool m_blur;
	bool m_castShadows;
	bool m_receiveShadows;

	float m_sunAngle;
	std::array<CcpMath::Sphere, 2> m_planets;
};

TYPEDEF_BLUECLASS( Tr2VolumetricsRenderer );
