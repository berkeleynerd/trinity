// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"

extern int g_dynamicExposureQualityRequirement;

namespace
{

constexpr uint64_t kPostProcessFingerprintOffset = 14695981039346656037ull;
constexpr uint64_t kPostProcessFingerprintPrime = 1099511628211ull;

void FingerprintBytes( uint64_t& hash, const void* value, size_t size )
{
	const auto* bytes = static_cast<const uint8_t*>( value );
	for( size_t index = 0; index < size; ++index )
	{
		hash ^= bytes[index];
		hash *= kPostProcessFingerprintPrime;
	}
}

template <typename T>
void FingerprintValue( uint64_t& hash, const T& value )
{
	FingerprintBytes( hash, &value, sizeof( value ) );
}

void FingerprintColor( uint64_t& hash, const Color& value )
{
	FingerprintValue( hash, value.r );
	FingerprintValue( hash, value.g );
	FingerprintValue( hash, value.b );
	FingerprintValue( hash, value.a );
}

uint64_t FingerprintFog( const Tr2PPFogEffect& fog )
{
	uint64_t hash = kPostProcessFingerprintOffset;
	FingerprintColor( hash, fog.m_color );
	const float values[] = {
		fog.m_nebulaInfluence,
		fog.m_nebulaBlur,
		fog.m_originalBrightenOnly,
		fog.m_colorInfluence,
		fog.m_totalAmount,
		fog.m_totalPower,
		fog.m_backgroundOcclusion,
		fog.m_intensity,
		fog.m_brightnessThreshold0,
		fog.m_brightnessThreshold1,
		fog.m_brightnessAdjustmentAmount,
		fog.m_blendDistance0,
		fog.m_blendBias0,
		fog.m_blendAmount0,
		fog.m_blendPower0,
		fog.m_blendDistance1,
		fog.m_blendBias1,
		fog.m_blendAmount1,
		fog.m_blendPower1,
		fog.m_blendDistance2,
		fog.m_blendBias2,
		fog.m_blendAmount2,
		fog.m_blendPower2,
		fog.m_areaSize.x,
		fog.m_areaSize.y,
		fog.m_areaSize.z,
		fog.m_areaScale.x,
		fog.m_areaScale.y,
		fog.m_areaCenter.x,
		fog.m_areaCenter.y,
		fog.m_areaCenter.z,
	};
	for( const float value : values )
	{
		FingerprintValue( hash, value );
	}
	return hash;
}

uint64_t FingerprintBloom( const Tr2PPBloomEffect& bloom )
{
	uint64_t hash = kPostProcessFingerprintOffset;
	const float values[] = {
		bloom.m_luminanceThreshold,
		bloom.m_luminanceScale,
		bloom.m_bloomBrightness,
		bloom.m_sizeScale,
		bloom.m_directionalWeight,
		bloom.m_grimeWeight,
	};
	for( const float value : values )
	{
		FingerprintValue( hash, value );
	}
	const uint8_t exposureDependency = bloom.m_exposureDependency ? 1u : 0u;
	FingerprintValue( hash, exposureDependency );
	FingerprintValue( hash, bloom.m_steps );
	for( const float value : bloom.m_stepSizes )
	{
		FingerprintValue( hash, value );
	}
	for( const Color& value : bloom.m_stepTints )
	{
		FingerprintColor( hash, value );
	}
	const char* grimePath = bloom.m_grimePath.c_str();
	FingerprintBytes( hash, grimePath, std::strlen( grimePath ) );
	return hash;
}

uint64_t FingerprintDesaturate( const Tr2PPDesaturateEffect& desaturate )
{
	uint64_t hash = kPostProcessFingerprintOffset;
	FingerprintValue( hash, desaturate.m_intensity );
	return hash;
}

} // namespace

#define RETURN_IF_ACTIVE( effect, currentQuality, minQuality )         \
	if( effect && effect->IsActive() && currentQuality >= minQuality ) \
	{                                                                  \
		return effect;                                                 \
	}                                                                  \
	return nullptr;

Tr2PostProcess2::Tr2PostProcess2( IRoot* lockobj ) :
	PARENTLOCK( m_luts )
{
}


Tr2PostProcess2::~Tr2PostProcess2()
{
}


Tr2PostProcess2::SelectedMemberDiagnostics Tr2PostProcess2::GetSelectedMemberDiagnostics() const
{
	return GetMemberDiagnosticsForDiagnostics( m_fog, m_bloom, m_desaturate );
}


Tr2PostProcess2::SelectedMemberDiagnostics Tr2PostProcess2::GetMemberDiagnosticsForDiagnostics(
	Tr2PPFogEffectPtr fog,
	Tr2PPBloomEffectPtr bloom,
	Tr2PPDesaturateEffectPtr desaturate )
{
	SelectedMemberDiagnostics diagnostics;
	diagnostics.hasFog = static_cast<bool>( fog );
	diagnostics.hasBloom = static_cast<bool>( bloom );
	diagnostics.hasDesaturate = static_cast<bool>( desaturate );
	if( fog )
	{
		diagnostics.fogFingerprint = FingerprintFog( *fog );
	}
	if( bloom )
	{
		diagnostics.bloomFingerprint = FingerprintBloom( *bloom );
	}
	if( desaturate )
	{
		diagnostics.desaturateFingerprint = FingerprintDesaturate( *desaturate );
	}
	return diagnostics;
}


float Tr2PostProcess2::GetMipLodBias() const
{
	float taa_bias = 0.0f;
	if( m_taa )
	{
		taa_bias = m_taa->IsActive() ? -1.0f : 0.0f;
	}

	return taa_bias;
}

void Tr2PostProcess2::GetAvilableSortedLuts( std::vector<const Tr2PPLutEffect*>& container, PostProcess::Quality qualitySetting ) const
{
	if( qualitySetting < PostProcess::LOW )
	{
		// This will never happen, but it will open for a scenario where we need to have game specific quality settings for each post process effect
		return;
	}
	container.clear();
	if( m_lut && m_lut->IsActive() )
	{
		container.push_back( m_lut );
	}
	for( const auto& lut : m_luts )
	{
		if( lut->IsActive() )
		{
			container.push_back( lut );
		}
	}
	std::sort( container.begin(), container.end(), []( const Tr2PPLutEffect* a, const Tr2PPLutEffect* b ) { return a->m_influence < b->m_influence; } );
}

void Tr2PostProcess2::AddLut( Tr2PPLutEffectPtr effect )
{
	m_luts.Append( effect );
}

void Tr2PostProcess2::ClearLuts()
{
	m_luts.Clear();
}

Tr2PPSignalLossEffectPtr Tr2PostProcess2::GetSignalLossIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_signalLoss, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetSignalLoss( Tr2PPSignalLossEffectPtr effect )
{
	m_signalLoss = effect;
}

Tr2PPGodRaysEffectPtr Tr2PostProcess2::GetGodRaysIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_godRays, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetGodRays( Tr2PPGodRaysEffectPtr effect )
{
	m_godRays = effect;
}

Tr2PPBloomEffectPtr Tr2PostProcess2::GetBloomIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_bloom, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetBloom( Tr2PPBloomEffectPtr effect )
{
	m_bloom = effect;
}

Tr2PPDynamicExposureEffectPtr Tr2PostProcess2::GetDynamicExposureIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_dynamicExposure, qualitySetting, (PostProcess::Quality)g_dynamicExposureQualityRequirement );
}

void Tr2PostProcess2::SetDynamicExposure( Tr2PPDynamicExposureEffectPtr effect )
{
	m_dynamicExposure = effect;
}

Tr2PPFilmGrainEffectPtr Tr2PostProcess2::GetFilmGrainIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_filmGrain, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetFilmGrain( Tr2PPFilmGrainEffectPtr effect )
{
	m_filmGrain = effect;
}

Tr2PPDesaturateEffectPtr Tr2PostProcess2::GetDesaturateIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_desaturate, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetDesaturate( Tr2PPDesaturateEffectPtr effect )
{
	m_desaturate = effect;
}

Tr2PPFadeEffectPtr Tr2PostProcess2::GetFadeIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_fade, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetFade( Tr2PPFadeEffectPtr effect )
{
	m_fade = effect;
}

Tr2PPVignetteEffectPtr Tr2PostProcess2::GetVignetteIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_vignette, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetVignette( Tr2PPVignetteEffectPtr effect )
{
	m_vignette = effect;
}

Tr2PPFogEffectPtr Tr2PostProcess2::GetFogIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_fog, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetFog( Tr2PPFogEffectPtr effect )
{
	m_fog = effect;
}

Tr2PPTaaEffectPtr Tr2PostProcess2::GetTaaIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_taa, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetTaa( Tr2PPTaaEffectPtr effect )
{
	m_taa = effect;
}

Tr2PPDepthOfFieldEffectPtr Tr2PostProcess2::GetDepthOfFieldIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_depthOfField, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetDepthOfField( Tr2PPDepthOfFieldEffectPtr effect )
{
	m_depthOfField = effect;
}

Tr2PPTonemappingEffectPtr Tr2PostProcess2::GetTonemappingIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_tonemapping, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetTonemapping( Tr2PPTonemappingEffectPtr effect )
{
	m_tonemapping = effect;
}

Tr2PPColorCorrectionEffectPtr Tr2PostProcess2::GetColorCorrectionIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_colorCorrection, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetColorCorrection( Tr2PPColorCorrectionEffectPtr effect )
{
	m_colorCorrection = effect;
}

Tr2PPGenericEffectPtr Tr2PostProcess2::GetGenericEffectIfAvailable( PostProcess::Quality qualitySetting ) const
{
	if( m_generic )
	{
		RETURN_IF_ACTIVE( m_generic, qualitySetting, m_generic->m_quality );
	}
	return nullptr;
}

void Tr2PostProcess2::SetGenericEffect( Tr2PPGenericEffectPtr effect )
{
	m_generic = effect;
}
