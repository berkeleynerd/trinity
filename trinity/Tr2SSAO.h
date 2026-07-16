// Copyright © 2022 CCP ehf.

#pragma once

#include "Tr2GpuResourcePool.h"
#include "TriFrustum.h"
#include "ffx_cacao.h"

enum class SSAOQuality
{
	HIGHEST = 0,
	HIGH = 1,
	MEDIUM = 2,
	LOW = 3,
	LOWEST = 4,
};

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2SSAO ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	struct RuntimeDiagnostics
	{
		bool enabled = false;
		bool cortaoEnabled = false;
		bool cortaoBentNormal = false;
		bool cortaoInitialized = false;
		bool deterministicRandom = false;
		bool temporal = false;
		bool depthReady = false;
		bool normalReady = false;
		bool outputReady = false;
		bool passSucceeded = false;
		bool blurEnabled = false;
		bool downsampled = false;
		uint32_t quality = 0;
		uint32_t inputWidth = 0;
		uint32_t inputHeight = 0;
		uint32_t outputWidth = 0;
		uint32_t outputHeight = 0;
		uint32_t outputFormat = 0;
		uint64_t filterCount = 0;
		float strength = 0.0f;
		float radius = 0.0f;
		float maxBlockerSearchRadius = 0.0f;
		float mipBias = 0.0f;
	};

	Tr2SSAO( IRoot* lockobj = NULL );

	Tr2GpuResourcePool::Texture Filter( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal );

	void Enable( bool enable );
	void SetQuality( SSAOQuality quality, bool downsampled );
	void SetCortaoEnabled( bool enabled )
	{
		m_cortaoEnabled = enabled;
	}
	bool GetCortaoEnabled() const
	{
		return m_cortaoEnabled;
	}
	void SetCortaoBentNormal( bool enabled )
	{
		m_cortaoBentNormal = enabled;
	}
	bool GetCortaoBentNormal() const
	{
		return m_cortaoBentNormal;
	}
	void ResetRandomSeedForTesting( uint32_t seed );
	bool IsDeterministicRandomForTesting() const
	{
		return m_cortaoDeterministicRandom;
	}
	RuntimeDiagnostics GetRuntimeDiagnostics() const;

private:
	struct SSAOResources;
	static constexpr unsigned SSAO_PASS_COUNT = 4;

	struct Layer
	{
		bool enabled = true;
		SSAOQuality quality;
		bool downsampled;
		float zoomLevel;

		FFX_CACAO_Settings settings;
		Tr2EffectPtr effect;
	};

	HRESULT ApplyConstBuffer( unsigned pass, Tr2RenderContext& renderContext );
	Tr2GpuResourcePool::Texture PerformPass( const Layer& layer, const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, bool reuseNormals, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );

	Layer m_detail = { true, SSAOQuality::HIGHEST, false, 5.f };

	Tr2ConstantBufferAL m_constBuffers[SSAO_PASS_COUNT + 1]{};


	//CORTAO stuff
	struct CortaoPerObjectData
	{
		Vector4 resolution;

		Vector4 projectionParams;

		Vector4 unprojectParams;

		Vector2 depthParams;
		float radius;
		float normalBias;

		float maxApparentCircleRadiusCoefficient;
		float mipBias;
		float radiusMultiplier;
		float lookupOccluderRadiusScale;

		uint32_t randomVectorSeedX;
		uint32_t randomVectorSeedY;
		float randomAngleOffset;
		float inverseMaxSlopeWeight;

		Matrix normalMatrix;

		uint32_t mipCount;
		uint32_t padding0;
		uint32_t padding1;
		uint32_t padding2;
	};

	struct CortaoDownsamplePerObjectData
	{
		uint32_t width;
		uint32_t height;
		uint32_t mipLevel;
		uint32_t random;
	};

	bool m_cortaoEnabled;
	bool m_cortaoBentNormal;

	bool m_cortaoInitialized;
	Tr2EffectPtr m_cortaoEffect;
	Tr2EffectPtr m_cortaoBlurEffect;
	TriTextureResPtr m_cortaoLookupTable;
	Tr2ConstantBufferAL m_cortaoConstantBuffer;

	float m_cortaoStrength;
	float m_cortaoRadius;
	float m_cortaoMaxBlockerSearchRadius;
	float m_cortaoMipBias;

	bool m_cortaoBlur;


	uint32_t m_cortaoRandSeeds[4] = {};
	bool m_cortaoDeterministicRandom = false;
	uint32_t m_cortaoDeterministicRandomState = 0;
	RuntimeDiagnostics m_runtimeDiagnostics;

	uint32_t Hash( uint32_t n );

	Tr2GpuResourcePool::Texture ComputeCORTAO( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal );
};

TYPEDEF_BLUECLASS( Tr2SSAO );
