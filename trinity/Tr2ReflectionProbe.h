// Copyright © 2019 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"
#include <array>
#include <vector>
#include "TriFrustum.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITriTextureRes );

BLUE_CLASS( Tr2ReflectionProbe ) :
	public INotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	enum ReflectionProbeRenderFrequency
	{
		ONE_SIDE_PER_FRAME,
		ALL_SIDES_PER_FRAME
	};

	struct RawTextureHashForTesting
	{
		uint32_t face = 0;
		uint32_t mip = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		Tr2RenderContextEnum::PixelFormat format = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
		uint64_t hash = 0;
		bool metricsAvailable = false;
		uint64_t finiteSamples = 0;
		uint64_t invalidSamples = 0;
		double minimumLuminance = 0.0;
		double meanLuminance = 0.0;
		double maximumLuminance = 0.0;
		double meanChroma = 0.0;
	};

	struct RawDiagnosticsForTesting
	{
		std::vector<RawTextureHashForTesting> source;
		std::vector<RawTextureHashForTesting> prefilter;
		std::vector<RawTextureHashForTesting> filtered;
		std::vector<RawTextureHashForTesting> sampled;
	};

	Tr2ReflectionProbe( IRoot* lockobj = NULL );
	~Tr2ReflectionProbe();

	void ReleaseResources( TriStorage s );
	bool OnPrepareResources();

	bool IsValid();
	bool HasData() const;
	bool LastFilterSucceeded() const;
	bool LastSamplingCopySucceeded() const;
	void InitRenderPass( Tr2RenderContext & renderContext );
	void StartRenderFace( unsigned face, Tr2RenderContext& renderContext );
	void EndRenderPass( Tr2RenderContext & renderContext );
	Tr2TextureAL GetDepthBuffer( unsigned face );

	TriFrustum GetFrustum( unsigned face, Tr2RenderContext& renderContext );

	Tr2RenderTargetPtr GetReflection();
	Tr2EffectPtr GetPreFilterEffect() const;
	Tr2EffectPtr GetFilterEffect() const;
	Tr2EffectPtr GetCopyMipEffect() const;
	ReflectionProbeRenderFrequency GetRenderFrequency() const;
	void SetRenderFrequency( ReflectionProbeRenderFrequency frequency );
	uint32_t GetReflectionWidth() const;
	uint32_t GetReflectionHeight() const;
	uint32_t GetReflectionMipCount() const;
	Vector3 GetPosition() const;
	bool GetLockPosition() const;
	bool GetHollywoodMode() const;
	Color GetBackLightColor() const;
	float GetBackLightContrast() const;
	bool CollectRawDiagnosticsForTesting(
		Tr2RenderContext & renderContext,
		RawDiagnosticsForTesting & diagnostics ) const;
	void SetPosition( Vector3 position );

	void SetBackLightColor( Color color );
	void SetIntensity( float intensity );
	void SetBackLightContrast( float contrast );

	bool OnModified( Be::Var * value );

	bool IsHollyWoodModeOn() const;
	bool ReadyForDynamicObjectReflections() const;

	uint8_t GetStartFace() const;
	uint8_t GetEndFace() const;

	const Tr2TextureAL& GetDepthBuffer( unsigned face ) const
	{
		return m_stencilMaps[face];
	}

private:
	void RunFilter();
	bool Filter( Tr2RenderContext & renderContext );
	bool CopyFilteredOutput( Tr2RenderContext & renderContext );
	bool DoPrepareResources( ImageIO::PixelFormat targetFormat, Tr2PrimaryRenderContext & renderContext );
	void DestroyRenderTargets();

	bool m_initialized;
	bool m_hasData;
	bool m_lastFilterSucceeded;
	bool m_lastSamplingCopySucceeded;
	bool m_lockPosition;
	Vector3 m_position;
	int m_intermediateSize;

	std::array<Tr2RenderTargetPtr, 6> m_renderTargets;
	std::array<Tr2TextureAL, 6> m_stencilMaps;
	Tr2RenderTargetPtr m_renderTargetCube;

	Tr2EffectPtr m_preFilterEffect;
	Tr2EffectPtr m_filterEffect;
	Tr2EffectPtr m_copyMipEffect;
	Tr2RenderTargetPtr m_preFilterTarget;
	Tr2RenderTargetPtr m_postFilterTarget;
	Tr2RenderTargetPtr m_samplingTarget;

	ITriTextureResPtr m_customSourceTexture;

	bool m_prevCullInversion;
	bool m_hdrOutput;
	ReflectionProbeRenderFrequency m_renderFrequency;
	uint8_t m_currentFrame;
	bool m_onePassDone;

	// Controls for hollywood lighting
	bool m_hollywoodMode;
	Color m_backlightColor;
	float m_backlightContrast;
};

TYPEDEF_BLUECLASS( Tr2ReflectionProbe );
