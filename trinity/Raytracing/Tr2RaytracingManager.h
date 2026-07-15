// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2RaytracingGeometry.h"
#include "Tr2Denoiser.h"
#include "Tr2ShadowMap.h"

class Tr2RtShaderTableAL;

BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2RaytracingManager ) :
	public IRoot
{
public:
	struct ShadowExecutionDiagnostics
	{
		bool effectReady = false;
		bool geometryPresent = false;
		bool techniquePresent = false;
		bool pipelineValid = false;
		bool shaderTableValid = false;
		bool dispatchAttempted = false;
		bool dispatchSucceeded = false;
		bool denoiserRequested = false;
		bool denoiserSucceeded = false;
		bool resultValid = false;
		uint64_t dispatchCount = 0;
		uint64_t denoiserCount = 0;
	};

	Tr2RaytracingManager( IRoot* lockobj = nullptr );
	~Tr2RaytracingManager();

	EXPOSE_TO_BLUE();

	Tr2RaytracingGeometry& GetGeometry();
	const ShadowExecutionDiagnostics& GetShadowExecutionDiagnostics() const
	{
		return m_shadowExecutionDiagnostics;
	}

	Tr2GpuResourcePool::Texture RenderShadows(
		const Tr2TextureAL& depth,
		const Tr2TextureAL& normal,
		const Vector3& sunDirection,
		const CcpMath::Sphere* planets,
		size_t planetCount,
		float upscaling,
		Tr2GpuResourcePool& gpuResourcePool,
		Tr2RenderContext& renderContext );

	bool OnPrepareResources();
	void ReleaseResources( TriStorage s );

	Tr2RaytracingPipelineStateManager m_pipelineManager;
	Tr2RtShaderTableDescriptionAL m_shaderTableDesc;

private:
	Tr2RaytracingGeometryPtr m_geometry;


	Tr2EffectPtr m_shadowEffect;
	unsigned m_shadowEffectHash;
	Tr2RtShaderTableAL m_shadowShaderTable;
	Tr2ConstantBufferAL m_shadowPerFrameData;

	// denoiser
	Tr2DenoiserPtr m_denoiser;

	float m_sunAngle;
	// debug
	bool m_applyDenoiser;
	ShadowExecutionDiagnostics m_shadowExecutionDiagnostics;
};

TYPEDEF_BLUECLASS( Tr2RaytracingManager );
