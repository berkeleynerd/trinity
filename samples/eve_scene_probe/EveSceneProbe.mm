// Copyright (c) 2026 CCP Games

#import <Cocoa/Cocoa.h>
#import <ImageIO/ImageIO.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "TrinityStandaloneProbeApi.h"

extern "C" bool TrinityStandaloneProbeStartup( int argc, const char* const* argv, const char* executableDirectory );
extern "C" void* TrinityStandaloneProbeCreateDevice( void* windowHandle,
													 uint32_t renderWidth,
													 uint32_t renderHeight,
													 int shaderTier );
extern "C" void TrinityStandaloneProbeDestroyDevice( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeGetCapturedProduct( void* opaqueProbe,
														  const uint8_t** pixels,
														  uint32_t* width,
														  uint32_t* height,
														  uint32_t* pitch );
extern "C" bool TrinityStandaloneProbeGetHdrCompositeDiagnostics( void* opaqueProbe,
																  uint32_t* format,
																  uint32_t* width,
																  uint32_t* height,
																  uint64_t* rawHash,
																  uint64_t* finiteComponents,
																  uint64_t* nanComponents,
																  uint64_t* infComponents,
																  uint64_t* negativeComponents,
																  uint64_t* saturatedComponents,
																  uint64_t* pixelsAboveOne,
																  double* minimumLuminance,
																  double* meanLuminance,
																  double* maximumLuminance,
																  double* p50Luminance,
																  double* p95Luminance,
																  double* p99Luminance,
																  bool* validationPassed );
extern "C" bool TrinityStandaloneProbeValidateComposition( void* opaqueProbe );
extern "C" const char* TrinityStandaloneProbeGetCompositionValidationSummary( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeGetShadowDiagnostics( void* opaqueProbe,
															uint32_t* casterTests,
															uint32_t* acceptedCascades,
															uint32_t* committedBatches );
extern "C" bool TrinityStandaloneProbeGetLocalShadowDiagnostics( void* opaqueProbe,
																 uint32_t* eligibleLights,
																 uint32_t* selectedLights,
																 uint32_t* droppedLights,
																 uint32_t* packedEntries,
																 uint32_t* hazeEligible,
																 uint32_t* bannerEligible,
																 uint32_t* validationEligible,
																 uint32_t* pointLights,
																 uint32_t* spotLights,
																 uint32_t* facePasses,
																 uint32_t* casterTests,
																 uint32_t* acceptedCasterPasses,
																 uint32_t* committedBatches,
																 uint32_t* atlasFormat,
																 uint32_t* atlasWidth,
																 uint32_t* atlasHeight,
																 bool* allocationSucceeded,
																 bool* atlasValid );
extern "C" bool TrinityStandaloneProbeGetReflectionDiagnostics( void* opaqueProbe,
																int* source,
																uint32_t* format,
																uint32_t* width,
																uint32_t* height,
																uint32_t* mipCount,
																bool* hasData,
																bool* filterSucceeded,
																float* reflectionIntensity,
																float* shPrimaryIntensity,
																float* shSecondaryIntensity );
extern "C" bool TrinityStandaloneProbeInspectClientAssets( void* opaqueProbe, const char* reportPath );
extern "C" bool TrinityStandaloneProbeCreateEveScene( void* opaqueProbe,
													  int qualityRung,
													  const char* assetPath,
													  int materialView,
													  int materialMode,
													  int areaView,
													  const char* sceneResourcePath,
													  int sceneFixture,
													  int lightingView,
													  int shSource,
													  int localLights,
													  int localShadows,
													  int reflectionSource,
													  int reflectionCorrection,
													  int normalMapMode,
													  int distortionMode,
													  int cameraView,
													  int composition,
													  int planetLayers,
													  int cloudYear,
													  int cloudMonth,
													  int cloudDay,
													  int sunEffects,
													  int attachments,
													  int attachmentView,
													  int decals,
													  int decalView,
													  uint32_t killCount,
													  float modelYawDegrees,
													  int shadows,
													  int ambientOcclusion,
													  int aoMethod );
extern "C" bool TrinityStandaloneProbeRenderFrame( void* opaqueProbe,
												   int qualityRung,
												   int64_t realTime,
												   int64_t simTime,
												   int captureProducts );

namespace
{

constexpr uint32_t kDefaultWindowWidth = 640;
constexpr uint32_t kDefaultWindowHeight = 480;
constexpr int64_t kFrameTime = 166667;

bool g_shouldQuit = false;

enum class QualityRung
{
	Shell,
	Scene,
	Model,
	HdrBlit,
	HdrPost,
	HdrExposure,
	HdrFinish,
};

enum class MaterialView
{
	Lit,
	BaseColor,
	Normal,
	Roughness,
	Material,
	Glow,
	Dirt,
	Mask,
	Paint,
};

enum class MaterialMode
{
	Probe,
	EveV5,
};

enum class AreaView
{
	All,
	Distortion,
	Hull,
	Booster,
};

enum class SceneFixture
{
	Empty,
	Fitting,
	A01,
	NewEden,
};

enum class LightingView
{
	Combined,
	Direct,
	Sh,
	Local,
	Environment,
};

enum class LocalLights
{
	Off,
	Authored,
	Validation,
};

enum class LocalShadows
{
	Auto,
	Off,
	Authored,
	Validation,
};

enum class Attachments
{
	Auto,
	Off,
	Authored,
};

enum class AttachmentView
{
	All,
	Sprites,
	SpriteLines,
	Spotlights,
	Planes,
	Hazes,
	Banners,
};

enum class Decals
{
	Auto,
	Off,
	Authored,
};

enum class DecalView
{
	All,
	Standard,
	Logos,
	Killmarks,
};

enum class ShaderTier
{
	Medium,
	High,
};

enum class ReflectionCorrection
{
	Off,
	Client,
};

enum class ReflectionSource
{
	Auto,
	Off,
	Static,
	Dynamic,
};

enum class NormalMapMode
{
	Authored,
	Flat,
};

enum class RenderProduct
{
	Window,
	Color,
	Depth,
	Normal,
	Shadow,
	ShadowAtlas,
	LocalShadowAtlas,
	AmbientOcclusion,
	BentNormal,
	Reflection,
	HdrComposite,
	PostTonemap,
	Bloom,
	FinalPostprocess,
	Distortion,
	All,
};

enum class Shadows
{
	Auto,
	Off,
	Low,
	High,
};

enum class AmbientOcclusion
{
	Auto,
	Off,
	Low,
	Medium,
	High,
};

enum class AoMethod
{
	Cortao,
	Cacao,
};

enum class CameraView
{
	Model,
	Celestials,
	Planet,
};

enum class SceneComposition
{
	System,
	Cinematic,
};

enum class PlanetLayers
{
	Surface,
	Atmosphere,
	Clouds,
	All,
};

enum class SunEffects
{
	Auto,
	Off,
	Flare,
	GodRays,
	All,
};

constexpr int kCaptureColor = 1 << 0;
constexpr int kCaptureDepth = 1 << 1;
constexpr int kCaptureNormal = 1 << 2;
constexpr int kCaptureShadow = 1 << 3;
constexpr int kCaptureShadowAtlas = 1 << 4;
constexpr int kCaptureAmbientOcclusion = 1 << 5;
constexpr int kCaptureBentNormal = 1 << 6;
constexpr int kCaptureReflection = 1 << 7;
constexpr int kCaptureLocalShadowAtlas = 1 << 8;
constexpr int kCaptureFreezeScene = 1 << 9;
constexpr int kCaptureHdrComposite = 1 << 10;
constexpr int kCapturePostTonemap = 1 << 11;
constexpr int kCaptureToneContract = 1 << 12;
constexpr int kCaptureBloom = 1 << 13;
constexpr int kCaptureFinalPostprocess = 1 << 14;
constexpr int kCapturePostFinishContract = 1 << 15;
constexpr int kCaptureDistortion = 1 << 16;

enum class DynamicExposure
{
	Auto,
	Off,
	Client,
};

enum class PostFinishMode
{
	Auto,
	Off,
	Client,
};

enum class DistortionMode
{
	Auto,
	Off,
	Authored,
};

enum class ExposureSequence
{
	None,
	DarkToBright,
	BrightToDark,
};

enum class ShSource
{
	NewEdenCelestials,
	Validation,
	None,
};

struct Options
{
	std::string asset = "astero";
	std::string inputPath;
	std::string capturePrefix;
	std::string inspectionReportPath;
	QualityRung qualityRung = QualityRung::Model;
	MaterialView materialView = MaterialView::Lit;
	MaterialMode materialMode = MaterialMode::Probe;
	AreaView areaView = AreaView::All;
	SceneFixture sceneFixture = SceneFixture::NewEden;
	LightingView lightingView = LightingView::Combined;
	ShSource shSource = ShSource::NewEdenCelestials;
	LocalLights localLights = LocalLights::Off;
	LocalShadows localShadows = LocalShadows::Auto;
	LocalShadows resolvedLocalShadows = LocalShadows::Off;
	Attachments attachments = Attachments::Auto;
	Attachments resolvedAttachments = Attachments::Off;
	AttachmentView attachmentView = AttachmentView::All;
	Decals decals = Decals::Auto;
	Decals resolvedDecals = Decals::Off;
	DecalView decalView = DecalView::All;
	uint32_t killCount = 0;
	float modelYawDegrees = 0.0f;
	ShaderTier shaderTier = ShaderTier::High;
	ReflectionCorrection reflectionCorrection = ReflectionCorrection::Client;
	ReflectionSource reflectionSource = ReflectionSource::Auto;
	ReflectionSource resolvedReflectionSource = ReflectionSource::Off;
	NormalMapMode normalMapMode = NormalMapMode::Authored;
	RenderProduct renderProduct = RenderProduct::Window;
	Shadows shadows = Shadows::Auto;
	Shadows resolvedShadows = Shadows::Off;
	AmbientOcclusion ambientOcclusion = AmbientOcclusion::Auto;
	AmbientOcclusion resolvedAmbientOcclusion = AmbientOcclusion::Off;
	AoMethod aoMethod = AoMethod::Cortao;
	CameraView cameraView = CameraView::Model;
	SceneComposition composition = SceneComposition::Cinematic;
	PlanetLayers planetLayers = PlanetLayers::All;
	SunEffects sunEffects = SunEffects::Auto;
	SunEffects resolvedSunEffects = SunEffects::Off;
	int cloudYear = 2026;
	int cloudMonth = 7;
	int cloudDay = 10;
	int maxFrames = -1;
	uint32_t windowWidth = kDefaultWindowWidth;
	uint32_t windowHeight = kDefaultWindowHeight;
	bool windowed = false;
	bool materialModeExplicit = false;
	bool localLightsExplicit = false;
	bool localShadowsExplicit = false;
	bool backgroundCapture = false;
	bool validateComposition = false;
	bool validateExposureTone = false;
	bool validatePostFinish = false;
	bool validateDistortion = false;
	bool framePacingCheck = false;
	uint32_t timingWarmup = 180;
	DynamicExposure dynamicExposure = DynamicExposure::Auto;
	DynamicExposure resolvedDynamicExposure = DynamicExposure::Off;
	PostFinishMode bloom = PostFinishMode::Auto;
	PostFinishMode resolvedBloom = PostFinishMode::Off;
	PostFinishMode filmGrain = PostFinishMode::Auto;
	PostFinishMode resolvedFilmGrain = PostFinishMode::Off;
	DistortionMode distortion = DistortionMode::Auto;
	DistortionMode resolvedDistortion = DistortionMode::Off;
	ExposureSequence exposureSequence = ExposureSequence::None;
	uint32_t exposureHold = 180;
};

std::string ToLower( std::string value );

std::string AreaViewName( AreaView view )
{
	switch( view )
	{
	case AreaView::All:
		return "all";
	case AreaView::Distortion:
		return "distortion";
	case AreaView::Hull:
		return "hull";
	case AreaView::Booster:
		return "booster";
	}
	return "unknown";
}

bool ParseAreaView( const std::string& value, AreaView& view )
{
	const std::string normalized = ToLower( value );
	const AreaView values[] = { AreaView::All, AreaView::Distortion, AreaView::Hull, AreaView::Booster };
	for( AreaView candidate : values )
	{
		if( normalized == AreaViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	return false;
}

bool ParseSceneFixture( const std::string& value, SceneFixture& fixture )
{
	const std::string lower = ToLower( value );
	if( lower == "empty" )
	{
		fixture = SceneFixture::Empty;
		return true;
	}
	if( lower == "fitting" )
	{
		fixture = SceneFixture::Fitting;
		return true;
	}
	if( lower == "a01" )
	{
		fixture = SceneFixture::A01;
		return true;
	}
	if( lower == "new-eden" || lower == "neweden" )
	{
		fixture = SceneFixture::NewEden;
		return true;
	}
	return false;
}

std::string LightingViewName( LightingView view )
{
	switch( view )
	{
	case LightingView::Combined:
		return "combined";
	case LightingView::Direct:
		return "direct";
	case LightingView::Sh:
		return "sh";
	case LightingView::Local:
		return "local";
	case LightingView::Environment:
		return "environment";
	}
	return "unknown";
}

bool ParseLightingView( const std::string& value, LightingView& view )
{
	const std::string normalized = ToLower( value );
	const LightingView values[] = {
		LightingView::Combined, LightingView::Direct, LightingView::Sh, LightingView::Local, LightingView::Environment
	};
	for( LightingView candidate : values )
	{
		if( normalized == LightingViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	return false;
}

std::string LocalLightsName( LocalLights mode )
{
	switch( mode )
	{
	case LocalLights::Off:
		return "off";
	case LocalLights::Authored:
		return "authored";
	case LocalLights::Validation:
		return "validation";
	}
	return "unknown";
}

bool ParseLocalLights( const std::string& value, LocalLights& mode )
{
	const std::string normalized = ToLower( value );
	const LocalLights values[] = { LocalLights::Off, LocalLights::Authored, LocalLights::Validation };
	for( LocalLights candidate : values )
	{
		if( normalized == LocalLightsName( candidate ) )
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}

std::string LocalShadowsName( LocalShadows mode )
{
	switch( mode )
	{
	case LocalShadows::Auto:
		return "auto";
	case LocalShadows::Off:
		return "off";
	case LocalShadows::Authored:
		return "authored";
	case LocalShadows::Validation:
		return "validation";
	}
	return "unknown";
}

bool ParseLocalShadows( const std::string& value, LocalShadows& mode )
{
	const std::string normalized = ToLower( value );
	const LocalShadows values[] = {
		LocalShadows::Auto, LocalShadows::Off, LocalShadows::Authored, LocalShadows::Validation
	};
	for( LocalShadows candidate : values )
	{
		if( normalized == LocalShadowsName( candidate ) )
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}

int LocalShadowsApiValue( LocalShadows mode )
{
	switch( mode )
	{
	case LocalShadows::Off:
		return 0;
	case LocalShadows::Authored:
		return 1;
	case LocalShadows::Validation:
		return 2;
	case LocalShadows::Auto:
		break;
	}
	return 0;
}

std::string AttachmentsName( Attachments mode )
{
	switch( mode )
	{
	case Attachments::Auto:
		return "auto";
	case Attachments::Off:
		return "off";
	case Attachments::Authored:
		return "authored";
	}
	return "unknown";
}

bool ParseAttachments( const std::string& value, Attachments& mode )
{
	const std::string normalized = ToLower( value );
	const Attachments values[] = { Attachments::Auto, Attachments::Off, Attachments::Authored };
	for( Attachments candidate : values )
	{
		if( normalized == AttachmentsName( candidate ) )
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}

std::string AttachmentViewName( AttachmentView view )
{
	static const char* names[] = { "all", "sprites", "sprite-lines", "spotlights", "planes", "hazes", "banners" };
	return names[static_cast<int>( view )];
}

bool ParseAttachmentView( const std::string& value, AttachmentView& view )
{
	const std::string normalized = ToLower( value );
	for( int candidate = 0; candidate <= static_cast<int>( AttachmentView::Banners ); ++candidate )
	{
		const auto candidateView = static_cast<AttachmentView>( candidate );
		if( normalized == AttachmentViewName( candidateView ) )
		{
			view = candidateView;
			return true;
		}
	}
	return false;
}

std::string DecalsName( Decals mode )
{
	switch( mode )
	{
	case Decals::Auto:
		return "auto";
	case Decals::Off:
		return "off";
	case Decals::Authored:
		return "authored";
	}
	return "unknown";
}

bool ParseDecals( const std::string& value, Decals& mode )
{
	const std::string normalized = ToLower( value );
	const Decals values[] = { Decals::Auto, Decals::Off, Decals::Authored };
	for( Decals candidate : values )
	{
		if( normalized == DecalsName( candidate ) )
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}

std::string DecalViewName( DecalView view )
{
	static const char* names[] = { "all", "standard", "logos", "killmarks" };
	return names[static_cast<int>( view )];
}

bool ParseDecalView( const std::string& value, DecalView& view )
{
	const std::string normalized = ToLower( value );
	for( int candidate = 0; candidate <= static_cast<int>( DecalView::Killmarks ); ++candidate )
	{
		const auto candidateView = static_cast<DecalView>( candidate );
		if( normalized == DecalViewName( candidateView ) )
		{
			view = candidateView;
			return true;
		}
	}
	return false;
}

std::string ShaderTierName( ShaderTier tier )
{
	return tier == ShaderTier::High ? "high" : "medium";
}

bool ParseShaderTier( const std::string& value, ShaderTier& tier )
{
	const std::string normalized = ToLower( value );
	if( normalized == "medium" || normalized == "hi" || normalized == "sm_3_0_hi" )
	{
		tier = ShaderTier::Medium;
		return true;
	}
	if( normalized == "high" || normalized == "depth" || normalized == "sm_3_0_depth" )
	{
		tier = ShaderTier::High;
		return true;
	}
	return false;
}

std::string ReflectionCorrectionName( ReflectionCorrection mode )
{
	return mode == ReflectionCorrection::Client ? "client" : "off";
}

bool ParseReflectionCorrection( const std::string& value, ReflectionCorrection& mode )
{
	const std::string normalized = ToLower( value );
	if( normalized == "off" )
	{
		mode = ReflectionCorrection::Off;
		return true;
	}
	if( normalized == "client" || normalized == "on" )
	{
		mode = ReflectionCorrection::Client;
		return true;
	}
	return false;
}

std::string ReflectionSourceName( ReflectionSource source )
{
	switch( source )
	{
	case ReflectionSource::Auto:
		return "auto";
	case ReflectionSource::Off:
		return "off";
	case ReflectionSource::Static:
		return "static";
	case ReflectionSource::Dynamic:
		return "dynamic";
	}
	return "unknown";
}

bool ParseReflectionSource( const std::string& value, ReflectionSource& source )
{
	const std::string normalized = ToLower( value );
	const ReflectionSource values[] = {
		ReflectionSource::Auto, ReflectionSource::Off, ReflectionSource::Static, ReflectionSource::Dynamic
	};
	for( ReflectionSource candidate : values )
	{
		if( normalized == ReflectionSourceName( candidate ) )
		{
			source = candidate;
			return true;
		}
	}
	return false;
}

int ReflectionSourceApiValue( ReflectionSource source )
{
	switch( source )
	{
	case ReflectionSource::Off:
		return 0;
	case ReflectionSource::Static:
		return 1;
	case ReflectionSource::Dynamic:
		return 2;
	case ReflectionSource::Auto:
		break;
	}
	return 0;
}

std::string NormalMapModeName( NormalMapMode mode )
{
	return mode == NormalMapMode::Flat ? "flat" : "authored";
}

bool ParseNormalMapMode( const std::string& value, NormalMapMode& mode )
{
	const std::string normalized = ToLower( value );
	if( normalized == "authored" )
	{
		mode = NormalMapMode::Authored;
		return true;
	}
	if( normalized == "flat" )
	{
		mode = NormalMapMode::Flat;
		return true;
	}
	return false;
}

std::string RenderProductName( RenderProduct product )
{
	switch( product )
	{
	case RenderProduct::Window:
		return "window";
	case RenderProduct::Color:
		return "color";
	case RenderProduct::Depth:
		return "depth";
	case RenderProduct::Normal:
		return "normal";
	case RenderProduct::Shadow:
		return "shadow";
	case RenderProduct::ShadowAtlas:
		return "shadow-atlas";
	case RenderProduct::LocalShadowAtlas:
		return "local-shadow-atlas";
	case RenderProduct::AmbientOcclusion:
		return "ao";
	case RenderProduct::BentNormal:
		return "bent-normal";
	case RenderProduct::Reflection:
		return "reflection";
	case RenderProduct::HdrComposite:
		return "hdr-composite";
	case RenderProduct::PostTonemap:
		return "post-tonemap";
	case RenderProduct::Bloom:
		return "bloom";
	case RenderProduct::FinalPostprocess:
		return "final-postprocess";
	case RenderProduct::Distortion:
		return "distortion";
	case RenderProduct::All:
		return "all";
	}
	return "unknown";
}

bool ParseRenderProduct( const std::string& value, RenderProduct& product )
{
	const std::string normalized = ToLower( value );
	const RenderProduct values[] = { RenderProduct::Window,
									 RenderProduct::Color,
									 RenderProduct::Depth,
									 RenderProduct::Normal,
									 RenderProduct::Shadow,
									 RenderProduct::ShadowAtlas,
									 RenderProduct::LocalShadowAtlas,
									 RenderProduct::AmbientOcclusion,
									 RenderProduct::BentNormal,
									 RenderProduct::Reflection,
									 RenderProduct::HdrComposite,
									 RenderProduct::PostTonemap,
									 RenderProduct::Bloom,
									 RenderProduct::FinalPostprocess,
									 RenderProduct::Distortion,
									 RenderProduct::All };
	for( RenderProduct candidate : values )
	{
		if( normalized == RenderProductName( candidate ) )
		{
			product = candidate;
			return true;
		}
	}
	return false;
}

std::string DynamicExposureName( DynamicExposure value )
{
	static const char* names[] = { "auto", "off", "client" };
	return names[static_cast<int>( value )];
}

bool ParseDynamicExposure( const std::string& value, DynamicExposure& result )
{
	const std::string normalized = ToLower( value );
	for( DynamicExposure candidate : { DynamicExposure::Auto, DynamicExposure::Off, DynamicExposure::Client } )
	{
		if( normalized == DynamicExposureName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string PostFinishModeName( PostFinishMode value )
{
	static const char* names[] = { "auto", "off", "client" };
	return names[static_cast<int>( value )];
}

bool ParsePostFinishMode( const std::string& value, PostFinishMode& result )
{
	const std::string normalized = ToLower( value );
	for( PostFinishMode candidate : { PostFinishMode::Auto, PostFinishMode::Off, PostFinishMode::Client } )
	{
		if( normalized == PostFinishModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string DistortionModeName( DistortionMode value )
{
	static const char* names[] = { "auto", "off", "authored" };
	return names[static_cast<int>( value )];
}

bool ParseDistortionMode( const std::string& value, DistortionMode& result )
{
	const std::string normalized = ToLower( value );
	for( DistortionMode candidate : { DistortionMode::Auto, DistortionMode::Off, DistortionMode::Authored } )
	{
		if( normalized == DistortionModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string ExposureSequenceName( ExposureSequence value )
{
	static const char* names[] = { "none", "dark-to-bright", "bright-to-dark" };
	return names[static_cast<int>( value )];
}

bool ParseExposureSequence( const std::string& value, ExposureSequence& result )
{
	const std::string normalized = ToLower( value );
	for( ExposureSequence candidate :
		 { ExposureSequence::None, ExposureSequence::DarkToBright, ExposureSequence::BrightToDark } )
	{
		if( normalized == ExposureSequenceName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string ShadowsName( Shadows shadows )
{
	static const char* names[] = { "auto", "off", "low", "high" };
	return names[static_cast<int>( shadows )];
}

bool ParseShadows( const std::string& value, Shadows& shadows )
{
	const std::string normalized = ToLower( value );
	for( int candidate = 0; candidate <= static_cast<int>( Shadows::High ); ++candidate )
	{
		const auto candidateValue = static_cast<Shadows>( candidate );
		if( normalized == ShadowsName( candidateValue ) )
		{
			shadows = candidateValue;
			return true;
		}
	}
	return false;
}

std::string AmbientOcclusionName( AmbientOcclusion ao )
{
	static const char* names[] = { "auto", "off", "low", "medium", "high" };
	return names[static_cast<int>( ao )];
}

bool ParseAmbientOcclusion( const std::string& value, AmbientOcclusion& ao )
{
	const std::string normalized = ToLower( value );
	for( int candidate = 0; candidate <= static_cast<int>( AmbientOcclusion::High ); ++candidate )
	{
		const auto candidateValue = static_cast<AmbientOcclusion>( candidate );
		if( normalized == AmbientOcclusionName( candidateValue ) )
		{
			ao = candidateValue;
			return true;
		}
	}
	return false;
}

std::string AoMethodName( AoMethod method )
{
	return method == AoMethod::Cortao ? "cortao" : "cacao";
}

bool ParseAoMethod( const std::string& value, AoMethod& method )
{
	const std::string normalized = ToLower( value );
	if( normalized == "cortao" )
	{
		method = AoMethod::Cortao;
		return true;
	}
	if( normalized == "cacao" )
	{
		method = AoMethod::Cacao;
		return true;
	}
	return false;
}

std::string CameraViewName( CameraView view )
{
	switch( view )
	{
	case CameraView::Model:
		return "model";
	case CameraView::Celestials:
		return "celestials";
	case CameraView::Planet:
		return "planet";
	}
	return "unknown";
}

bool ParseCameraView( const std::string& value, CameraView& view )
{
	const std::string normalized = ToLower( value );
	if( normalized == "model" )
	{
		view = CameraView::Model;
		return true;
	}
	if( normalized == "celestials" )
	{
		view = CameraView::Celestials;
		return true;
	}
	if( normalized == "planet" )
	{
		view = CameraView::Planet;
		return true;
	}
	return false;
}

std::string SceneCompositionName( SceneComposition composition )
{
	return composition == SceneComposition::Cinematic ? "cinematic" : "system";
}

bool ParseSceneComposition( const std::string& value, SceneComposition& composition )
{
	const std::string normalized = ToLower( value );
	if( normalized == "system" )
	{
		composition = SceneComposition::System;
		return true;
	}
	if( normalized == "cinematic" )
	{
		composition = SceneComposition::Cinematic;
		return true;
	}
	return false;
}

std::string PlanetLayersName( PlanetLayers layers )
{
	switch( layers )
	{
	case PlanetLayers::Surface:
		return "surface";
	case PlanetLayers::Atmosphere:
		return "atmosphere";
	case PlanetLayers::Clouds:
		return "clouds";
	case PlanetLayers::All:
		return "all";
	}
	return "unknown";
}

bool ParsePlanetLayers( const std::string& value, PlanetLayers& layers )
{
	const std::string normalized = ToLower( value );
	const PlanetLayers values[] = {
		PlanetLayers::Surface, PlanetLayers::Atmosphere, PlanetLayers::Clouds, PlanetLayers::All
	};
	for( PlanetLayers candidate : values )
	{
		if( normalized == PlanetLayersName( candidate ) )
		{
			layers = candidate;
			return true;
		}
	}
	return false;
}

std::string SunEffectsName( SunEffects effects )
{
	switch( effects )
	{
	case SunEffects::Auto:
		return "auto";
	case SunEffects::Off:
		return "off";
	case SunEffects::Flare:
		return "flare";
	case SunEffects::GodRays:
		return "god-rays";
	case SunEffects::All:
		return "all";
	}
	return "unknown";
}

bool ParseSunEffects( const std::string& value, SunEffects& effects )
{
	const std::string normalized = ToLower( value );
	const SunEffects values[] = {
		SunEffects::Auto, SunEffects::Off, SunEffects::Flare, SunEffects::GodRays, SunEffects::All
	};
	for( SunEffects candidate : values )
	{
		if( normalized == SunEffectsName( candidate ) )
		{
			effects = candidate;
			return true;
		}
	}
	return false;
}

std::string PlanetCloudDateName( const Options& options )
{
	char value[16];
	std::snprintf( value, sizeof( value ), "%04d-%02d-%02d", options.cloudYear, options.cloudMonth, options.cloudDay );
	return value;
}

bool ParsePlanetCloudDate( const std::string& value, Options& options )
{
	NSInteger year = 0;
	NSInteger month = 0;
	NSInteger day = 0;
	NSCalendar* calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
	if( ToLower( value ) == "today" )
	{
		NSDateComponents* components = [calendar components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay
												   fromDate:[NSDate date]];
		year = components.year;
		month = components.month;
		day = components.day;
	}
	else
	{
		char trailing = '\0';
		int parsedYear = 0;
		int parsedMonth = 0;
		int parsedDay = 0;
		if( std::sscanf( value.c_str(), "%d-%d-%d%c", &parsedYear, &parsedMonth, &parsedDay, &trailing ) != 3 )
		{
			return false;
		}
		NSDateComponents* requested = [[NSDateComponents alloc] init];
		requested.year = parsedYear;
		requested.month = parsedMonth;
		requested.day = parsedDay;
		NSDate* date = [calendar dateFromComponents:requested];
		if( !date )
		{
			return false;
		}
		NSDateComponents* normalized = [calendar components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay
												   fromDate:date];
		if( normalized.year != parsedYear || normalized.month != parsedMonth || normalized.day != parsedDay )
		{
			return false;
		}
		year = normalized.year;
		month = normalized.month;
		day = normalized.day;
	}
	options.cloudYear = static_cast<int>( year );
	options.cloudMonth = static_cast<int>( month );
	options.cloudDay = static_cast<int>( day );
	return true;
}

int RenderProductApiValue( RenderProduct product )
{
	switch( product )
	{
	case RenderProduct::Color:
		return kCaptureColor;
	case RenderProduct::Depth:
		return kCaptureDepth;
	case RenderProduct::Normal:
		return kCaptureNormal;
	case RenderProduct::Shadow:
		return kCaptureShadow;
	case RenderProduct::ShadowAtlas:
		return kCaptureShadowAtlas;
	case RenderProduct::LocalShadowAtlas:
		return kCaptureLocalShadowAtlas;
	case RenderProduct::AmbientOcclusion:
		return kCaptureAmbientOcclusion;
	case RenderProduct::BentNormal:
		return kCaptureBentNormal;
	case RenderProduct::Reflection:
		return kCaptureReflection;
	case RenderProduct::HdrComposite:
		return kCaptureHdrComposite;
	case RenderProduct::PostTonemap:
		return kCapturePostTonemap;
	case RenderProduct::Bloom:
		return kCaptureBloom;
	case RenderProduct::FinalPostprocess:
		return kCaptureFinalPostprocess;
	case RenderProduct::Distortion:
		return kCaptureDistortion;
	case RenderProduct::All:
		return 0;
	case RenderProduct::Window:
		return 0;
	}
	return 0;
}

int ShadowsApiValue( Shadows shadows )
{
	switch( shadows )
	{
	case Shadows::Off:
		return 0;
	case Shadows::Low:
		return 1;
	case Shadows::High:
		return 2;
	case Shadows::Auto:
		break;
	}
	return 0;
}

int AmbientOcclusionApiValue( AmbientOcclusion ao )
{
	switch( ao )
	{
	case AmbientOcclusion::Off:
		return 0;
	case AmbientOcclusion::Low:
		return 1;
	case AmbientOcclusion::Medium:
		return 2;
	case AmbientOcclusion::High:
		return 3;
	case AmbientOcclusion::Auto:
		break;
	}
	return 0;
}

std::string RenderProductStats( const Options& options, RenderProduct product )
{
	switch( product )
	{
	case RenderProduct::Color:
		return "source=presented drawable gpuVisualization=false";
	case RenderProduct::Depth:
		return "source=DepthMap format=D32_FLOAT reverseZ=true clear=0 gpuVisualization=true";
	case RenderProduct::Normal:
		return "source=NormalMap format=R10G10B10A2_UNORM clear=0 gpuVisualization=true";
	case RenderProduct::Shadow:
		return "source=ShadowMap format=R8_UNORM channel=red gpuVisualization=true";
	case RenderProduct::ShadowAtlas:
		return "source=CascadedShadowDepth format=D32_FLOAT dimensions=16384x4096 gpuVisualization=true";
	case RenderProduct::LocalShadowAtlas:
		return "source=DynamicLightShadowDepth format=D32_FLOAT dimensions=16384x16384 gpuVisualization=true";
	case RenderProduct::AmbientOcclusion:
		return options.aoMethod == AoMethod::Cortao ?
			"source=SSAOMap format=R8G8B8A8_SNORM channel=alpha-snorm-remapped gpuVisualization=true" :
			"source=SSAOMap format=R8_UNORM channel=red gpuVisualization=true";
	case RenderProduct::BentNormal:
		return "source=SSAOMap format=R8G8B8A8_SNORM channel=rgb-remapped gpuVisualization=true";
	case RenderProduct::Reflection:
		return "source=EveSpaceSceneEnvMap type=cube layout=3x2 toneMapped=true gpuVisualization=true";
	case RenderProduct::HdrComposite:
		return "source=PreTonemapColor format=R16G16B16A16_FLOAT visualization=reinhard-plus-gamma diagnosticOnly=true gpuVisualization=true rawValidation=true";
	case RenderProduct::PostTonemap:
		return "source=PostTonemapColor format=destination-8-bit-unorm visualization=identity gpuVisualization=true rawValidation=true";
	case RenderProduct::Bloom:
		return "source=BloomMap format=R16G16B16A16_FLOAT dimensions=half-resolution visualization=reinhard-plus-gamma gpuVisualization=true rawValidation=true";
	case RenderProduct::FinalPostprocess:
		return "source=FinalPostProcessColor format=destination-8-bit-unorm visualization=identity gpuVisualization=true rawValidation=true";
	case RenderProduct::Distortion:
		return "source=DistortionMap format=B8G8R8A8_UNORM visualization=decoded-rg-vector gpuVisualization=true rawValidation=true";
	case RenderProduct::Window:
	case RenderProduct::All:
		break;
	}
	return "source=presented drawable gpuVisualization=false";
}

std::string ShSourceName( ShSource source )
{
	switch( source )
	{
	case ShSource::NewEdenCelestials:
		return "new-eden-celestials";
	case ShSource::Validation:
		return "validation";
	case ShSource::None:
		return "none";
	}
	return "unknown";
}

bool ParseShSource( const std::string& value, ShSource& source )
{
	const std::string normalized = ToLower( value );
	if( normalized == "new-eden-planet" )
	{
		source = ShSource::NewEdenCelestials;
		return true;
	}
	const ShSource values[] = { ShSource::NewEdenCelestials, ShSource::Validation, ShSource::None };
	for( ShSource candidate : values )
	{
		if( normalized == ShSourceName( candidate ) )
		{
			source = candidate;
			return true;
		}
	}
	return false;
}

const char* SceneResourcePath( SceneFixture fixture )
{
	switch( fixture )
	{
	case SceneFixture::Empty:
		return "";
	case SceneFixture::Fitting:
		return "res:/dx9/scene/fitting/fitting.black";
	case SceneFixture::A01:
	case SceneFixture::NewEden:
		return "res:/dx9/scene/universe/a01_cube.black";
	}
	return "";
}

int SceneFixtureApiValue( SceneFixture fixture )
{
	return static_cast<int>( fixture );
}

std::string ToLower( std::string value )
{
	std::transform( value.begin(), value.end(), value.begin(), []( unsigned char c ) {
		return static_cast<char>( std::tolower( c ) );
	} );
	return value;
}

std::string MaterialModeName( MaterialMode mode )
{
	return mode == MaterialMode::EveV5 ? "eve-v5" : "probe";
}

bool ParseMaterialMode( const std::string& value, MaterialMode& mode )
{
	const std::string normalized = ToLower( value );
	if( normalized == "probe" )
	{
		mode = MaterialMode::Probe;
		return true;
	}
	if( normalized == "eve-v5" || normalized == "v5" || normalized == "quadv5" )
	{
		mode = MaterialMode::EveV5;
		return true;
	}
	return false;
}

std::string MaterialViewName( MaterialView view )
{
	switch( view )
	{
	case MaterialView::Lit:
		return "lit";
	case MaterialView::BaseColor:
		return "basecolor";
	case MaterialView::Normal:
		return "normal";
	case MaterialView::Roughness:
		return "roughness";
	case MaterialView::Material:
		return "material";
	case MaterialView::Glow:
		return "glow";
	case MaterialView::Dirt:
		return "d";
	case MaterialView::Mask:
		return "mask";
	case MaterialView::Paint:
		return "p3";
	}
	return "unknown";
}

bool ParseMaterialView( const std::string& value, MaterialView& view )
{
	const std::string normalized = ToLower( value );
	const MaterialView values[] = {
		MaterialView::Lit,       MaterialView::BaseColor, MaterialView::Normal,
		MaterialView::Roughness, MaterialView::Material,  MaterialView::Glow,
		MaterialView::Dirt,      MaterialView::Mask,      MaterialView::Paint,
	};
	for( MaterialView candidate : values )
	{
		if( normalized == MaterialViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	if( normalized == "dirt" )
	{
		view = MaterialView::Dirt;
		return true;
	}
	if( normalized == "paint" )
	{
		view = MaterialView::Paint;
		return true;
	}
	return false;
}
std::string QualityRungName( QualityRung rung )
{
	switch( rung )
	{
	case QualityRung::Shell:
		return "shell";
	case QualityRung::Scene:
		return "scene";
	case QualityRung::Model:
		return "model";
	case QualityRung::HdrBlit:
		return "hdr-blit";
	case QualityRung::HdrPost:
		return "hdr-post";
	case QualityRung::HdrExposure:
		return "hdr-exposure";
	case QualityRung::HdrFinish:
		return "hdr-finish";
	}
	return "unknown";
}

int QualityRungApiValue( QualityRung rung )
{
	switch( rung )
	{
	case QualityRung::Shell:
		return 0;
	case QualityRung::Scene:
		return 2;
	case QualityRung::Model:
		return 3;
	case QualityRung::HdrBlit:
		return 4;
	case QualityRung::HdrPost:
		return 5;
	case QualityRung::HdrExposure:
		return 6;
	case QualityRung::HdrFinish:
		return 7;
	}
	return 0;
}

bool ParseQualityRung( const std::string& value, QualityRung& rung )
{
	const std::string normalized = ToLower( value );
	if( normalized == "shell" || normalized == "rung1" || normalized == "1" )
	{
		rung = QualityRung::Shell;
		return true;
	}
	if( normalized == "scene" || normalized == "empty-scene" || normalized == "rung2" || normalized == "2" )
	{
		rung = QualityRung::Scene;
		return true;
	}
	if( normalized == "model" || normalized == "synthetic-model" || normalized == "rung3" || normalized == "3" )
	{
		rung = QualityRung::Model;
		return true;
	}
	if( normalized == "hdr-blit" || normalized == "blit" )
	{
		rung = QualityRung::HdrBlit;
		return true;
	}
	if( normalized == "hdr" || normalized == "hdr-post" || normalized == "post" || normalized == "rung4" ||
		normalized == "4" )
	{
		rung = QualityRung::HdrPost;
		return true;
	}
	if( normalized == "hdr-exposure" || normalized == "exposure" )
	{
		rung = QualityRung::HdrExposure;
		return true;
	}
	if( normalized == "hdr-finish" || normalized == "finish" || normalized == "rc11" )
	{
		rung = QualityRung::HdrFinish;
		return true;
	}
	return false;
}

bool ParseUnsigned( const std::string& text, uint32_t& value )
{
	char* end = nullptr;
	unsigned long parsed = std::strtoul( text.c_str(), &end, 10 );
	if( !end || *end != '\0' || parsed == 0 || parsed > UINT32_MAX )
	{
		return false;
	}
	value = static_cast<uint32_t>( parsed );
	return true;
}

bool ParseWindowSize( const std::string& text, uint32_t& width, uint32_t& height )
{
	size_t separator = text.find( 'x' );
	if( separator == std::string::npos )
	{
		separator = text.find( 'X' );
	}
	if( separator == std::string::npos )
	{
		return false;
	}

	return ParseUnsigned( text.substr( 0, separator ), width ) && ParseUnsigned( text.substr( separator + 1 ), height );
}

std::string ExecutableDirectory()
{
	NSString* executablePath = [[NSBundle mainBundle] executablePath];
	NSString* executableDirectory = [executablePath stringByDeletingLastPathComponent];
	return executableDirectory.UTF8String ? executableDirectory.UTF8String : ".";
}

std::string DefaultAssetPath( const Options& options )
{
	const std::string executableDirectory = ExecutableDirectory();
	if( options.asset == "fox" )
	{
		return executableDirectory + "/Assets/Fox.cmf";
	}
	if( options.asset == "astero" )
	{
		return executableDirectory + "/Assets/Astero.cmf";
	}
	return executableDirectory + "/Assets/11989_lite.cmf";
}

bool FileExists( const std::string& path )
{
	std::ifstream file( path, std::ios::binary );
	return file.good();
}

void PrintUsage( const char* executable )
{
	std::cerr
		<< "Usage: " << executable << " [--asset astero|ship|fox] [--input PATH] [--frames N]\n"
		<< "       [--windowed WxH] [--quality-rung shell|scene|model|hdr-blit|hdr-post|hdr-exposure|hdr-finish]\n"
		<< "       [--material-mode probe|eve-v5]\n"
		<< "       [--area-view all|hull|booster|distortion]\n"
		<< "       [--scene-fixture empty|fitting|a01|new-eden]\n"
		<< "       [--lighting-view combined|direct|sh|local|environment] [--sh-source new-eden-celestials|validation|none]\n"
		<< "       [--local-lights off|authored|validation] [--local-shadows auto|off|authored|validation]\n"
		<< "       [--attachments auto|off|authored] [--attachment-view all|sprites|sprite-lines|spotlights|planes|hazes|banners]\n"
		<< "       [--decals auto|off|authored] [--decal-view all|standard|logos|killmarks] [--kill-count N]\n"
		<< "       [--model-yaw-degrees N]\n"
		<< "       [--shader-tier medium|high]\n"
		<< "       [--reflection-source auto|off|static|dynamic]\n"
		<< "       [--reflection-correction off|client]\n"
		<< "       [--normal-map authored|flat]\n"
		<< "       [--render-product window|color|depth|normal|shadow|shadow-atlas|local-shadow-atlas|ao|bent-normal|reflection|hdr-composite|post-tonemap|bloom|final-postprocess|distortion|all]\n"
		<< "       [--shadows auto|off|low|high] [--ao auto|off|low|medium|high]\n"
		<< "       [--ao-method cortao|cacao]\n"
		<< "       [--camera-view model|celestials|planet]\n"
		<< "       [--composition system|cinematic] [--planet-layers surface|atmosphere|clouds|all]\n"
		<< "       [--sun-effects auto|off|flare|god-rays|all]\n"
		<< "       [--planet-cloud-date YYYY-MM-DD|today] [--background-capture]\n"
		<< "       [--dynamic-exposure auto|off|client] [--validate-exposure-tone]\n"
		<< "       [--bloom auto|off|client] [--film-grain auto|off|client] [--validate-post-finish]\n"
		<< "       [--distortion auto|off|authored] [--validate-distortion]\n"
		<< "       [--exposure-sequence none|dark-to-bright|bright-to-dark] [--exposure-hold N]\n"
		<< "       [--validate-composition] [--frame-pacing-check] [--timing-warmup N]\n"
		<< "       [--material-view lit|basecolor|normal|roughness|material|glow|d|mask|p3]\n"
		<< "       [--capture-prefix PATH] [--inspect-client-assets REPORT.md]\n";
}

bool ValidateCanonicalCompositionOptions( const Options& options )
{
	std::vector<std::string> mismatches;
	auto require = [&]( bool condition, const char* requirement ) {
		if( !condition )
		{
			mismatches.emplace_back( requirement );
		}
	};
	require( options.qualityRung == QualityRung::HdrPost || options.qualityRung == QualityRung::HdrExposure ||
				 options.qualityRung == QualityRung::HdrFinish,
			 "--quality-rung hdr-post, hdr-exposure, or hdr-finish" );
	require( options.asset == "astero", "--asset astero" );
	require( options.sceneFixture == SceneFixture::NewEden, "--scene-fixture new-eden" );
	require( options.cameraView == CameraView::Model, "--camera-view model" );
	require( options.composition == SceneComposition::Cinematic, "--composition cinematic" );
	require( options.cloudYear == 2026 && options.cloudMonth == 7 && options.cloudDay == 10,
			 "--planet-cloud-date 2026-07-10" );
	require( options.materialMode == MaterialMode::EveV5, "--material-mode eve-v5" );
	require( options.materialView == MaterialView::Lit, "--material-view lit" );
	require( options.areaView == AreaView::All, "--area-view all" );
	require( options.shaderTier == ShaderTier::High, "--shader-tier high" );
	require( options.normalMapMode == NormalMapMode::Authored, "--normal-map authored" );
	require( options.resolvedAttachments == Attachments::Authored && options.attachmentView == AttachmentView::All,
			 "--attachments authored --attachment-view all" );
	require( options.resolvedDecals == Decals::Authored && options.decalView == DecalView::All,
			 "--decals authored --decal-view all" );
	require( options.killCount == 0, "--kill-count 0" );
	require( options.lightingView == LightingView::Combined, "--lighting-view combined" );
	require( options.shSource == ShSource::NewEdenCelestials, "--sh-source new-eden-celestials" );
	require( options.localLights == LocalLights::Authored, "--local-lights authored" );
	require( options.resolvedLocalShadows == LocalShadows::Off, "--local-shadows off (or client-parity auto)" );
	require( options.resolvedReflectionSource == ReflectionSource::Dynamic, "--reflection-source dynamic" );
	require( options.reflectionCorrection == ReflectionCorrection::Client, "--reflection-correction client" );
	require( options.resolvedShadows == Shadows::High, "--shadows high" );
	require( options.resolvedAmbientOcclusion == AmbientOcclusion::High && options.aoMethod == AoMethod::Cortao,
			 "--ao high --ao-method cortao" );
	require( options.planetLayers == PlanetLayers::All, "--planet-layers all" );
	require( options.resolvedSunEffects == SunEffects::All, "--sun-effects all" );
	if( mismatches.empty() )
	{
		return true;
	}

	std::cerr << "--validate-composition requires the canonical RC-09 profile without option substitution:\n";
	for( const std::string& mismatch : mismatches )
	{
		std::cerr << "  " << mismatch << "\n";
	}
	return false;
}

bool ParseArgs( int argc, char** argv, Options& options )
{
	for( int i = 1; i < argc; ++i )
	{
		const std::string arg = argv[i];
		if( arg == "--asset" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.asset = ToLower( argv[i] );
			if( options.asset != "astero" && options.asset != "ship" && options.asset != "fox" )
			{
				return false;
			}
		}
		else if( arg == "--input" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.inputPath = argv[i];
		}
		else if( arg == "--frames" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			long parsed = std::strtol( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed < 0 || parsed > INT32_MAX )
			{
				return false;
			}
			options.maxFrames = static_cast<int>( parsed );
		}
		else if( arg == "--timing-warmup" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			const unsigned long parsed = std::strtoul( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed > UINT32_MAX )
			{
				return false;
			}
			options.timingWarmup = static_cast<uint32_t>( parsed );
		}
		else if( arg == "--exposure-hold" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			const unsigned long parsed = std::strtoul( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed == 0 || parsed > UINT32_MAX )
			{
				return false;
			}
			options.exposureHold = static_cast<uint32_t>( parsed );
		}
		else if( arg == "--dynamic-exposure" )
		{
			if( ++i >= argc || !ParseDynamicExposure( argv[i], options.dynamicExposure ) )
			{
				return false;
			}
		}
		else if( arg == "--bloom" )
		{
			if( ++i >= argc || !ParsePostFinishMode( argv[i], options.bloom ) )
			{
				return false;
			}
		}
		else if( arg == "--film-grain" )
		{
			if( ++i >= argc || !ParsePostFinishMode( argv[i], options.filmGrain ) )
			{
				return false;
			}
		}
		else if( arg == "--distortion" )
		{
			if( ++i >= argc || !ParseDistortionMode( argv[i], options.distortion ) )
			{
				return false;
			}
		}
		else if( arg == "--exposure-sequence" )
		{
			if( ++i >= argc || !ParseExposureSequence( argv[i], options.exposureSequence ) )
			{
				return false;
			}
		}
		else if( arg == "--validate-exposure-tone" )
		{
			options.validateExposureTone = true;
		}
		else if( arg == "--validate-post-finish" )
		{
			options.validatePostFinish = true;
		}
		else if( arg == "--validate-distortion" )
		{
			options.validateDistortion = true;
		}
		else if( arg == "--validate-composition" )
		{
			options.validateComposition = true;
		}
		else if( arg == "--frame-pacing-check" )
		{
			options.framePacingCheck = true;
		}
		else if( arg == "--background-capture" )
		{
			options.backgroundCapture = true;
		}
		else if( arg == "--windowed" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			if( !ParseWindowSize( argv[i], options.windowWidth, options.windowHeight ) )
			{
				return false;
			}
			options.windowed = true;
		}
		else if( arg == "--quality-rung" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			if( !ParseQualityRung( argv[i], options.qualityRung ) )
			{
				return false;
			}
		}
		else if( arg == "--capture-prefix" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.capturePrefix = argv[i];
		}
		else if( arg == "--material-view" )
		{
			if( ++i >= argc || !ParseMaterialView( argv[i], options.materialView ) )
			{
				return false;
			}
		}
		else if( arg == "--material-mode" )
		{
			if( ++i >= argc || !ParseMaterialMode( argv[i], options.materialMode ) )
			{
				return false;
			}
			options.materialModeExplicit = true;
		}
		else if( arg == "--area-view" )
		{
			if( ++i >= argc || !ParseAreaView( argv[i], options.areaView ) )
			{
				return false;
			}
		}
		else if( arg == "--scene-fixture" )
		{
			if( ++i >= argc || !ParseSceneFixture( argv[i], options.sceneFixture ) )
			{
				return false;
			}
		}
		else if( arg == "--lighting-view" )
		{
			if( ++i >= argc || !ParseLightingView( argv[i], options.lightingView ) )
			{
				return false;
			}
		}
		else if( arg == "--sh-source" )
		{
			if( ++i >= argc || !ParseShSource( argv[i], options.shSource ) )
			{
				return false;
			}
		}
		else if( arg == "--local-lights" )
		{
			if( ++i >= argc || !ParseLocalLights( argv[i], options.localLights ) )
			{
				return false;
			}
			options.localLightsExplicit = true;
		}
		else if( arg == "--local-shadows" )
		{
			if( ++i >= argc || !ParseLocalShadows( argv[i], options.localShadows ) )
			{
				return false;
			}
			options.localShadowsExplicit = true;
		}
		else if( arg == "--attachments" )
		{
			if( ++i >= argc || !ParseAttachments( argv[i], options.attachments ) )
			{
				return false;
			}
		}
		else if( arg == "--attachment-view" )
		{
			if( ++i >= argc || !ParseAttachmentView( argv[i], options.attachmentView ) )
			{
				return false;
			}
		}
		else if( arg == "--decals" )
		{
			if( ++i >= argc || !ParseDecals( argv[i], options.decals ) )
			{
				return false;
			}
		}
		else if( arg == "--decal-view" )
		{
			if( ++i >= argc || !ParseDecalView( argv[i], options.decalView ) )
			{
				return false;
			}
		}
		else if( arg == "--kill-count" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			const unsigned long long parsed = std::strtoull( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed > UINT32_MAX )
			{
				return false;
			}
			options.killCount = static_cast<uint32_t>( parsed );
		}
		else if( arg == "--model-yaw-degrees" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			const float parsed = std::strtof( argv[i], &end );
			if( !end || *end != '\0' || !std::isfinite( parsed ) )
			{
				return false;
			}
			options.modelYawDegrees = parsed;
		}
		else if( arg == "--shader-tier" )
		{
			if( ++i >= argc || !ParseShaderTier( argv[i], options.shaderTier ) )
			{
				return false;
			}
		}
		else if( arg == "--reflection-correction" )
		{
			if( ++i >= argc || !ParseReflectionCorrection( argv[i], options.reflectionCorrection ) )
			{
				return false;
			}
		}
		else if( arg == "--reflection-source" )
		{
			if( ++i >= argc || !ParseReflectionSource( argv[i], options.reflectionSource ) )
			{
				return false;
			}
		}
		else if( arg == "--normal-map" )
		{
			if( ++i >= argc || !ParseNormalMapMode( argv[i], options.normalMapMode ) )
			{
				return false;
			}
		}
		else if( arg == "--render-product" )
		{
			if( ++i >= argc || !ParseRenderProduct( argv[i], options.renderProduct ) )
			{
				return false;
			}
		}
		else if( arg == "--shadows" )
		{
			if( ++i >= argc || !ParseShadows( argv[i], options.shadows ) )
			{
				return false;
			}
		}
		else if( arg == "--ao" )
		{
			if( ++i >= argc || !ParseAmbientOcclusion( argv[i], options.ambientOcclusion ) )
			{
				return false;
			}
		}
		else if( arg == "--ao-method" )
		{
			if( ++i >= argc || !ParseAoMethod( argv[i], options.aoMethod ) )
			{
				return false;
			}
		}
		else if( arg == "--camera-view" )
		{
			if( ++i >= argc || !ParseCameraView( argv[i], options.cameraView ) )
			{
				return false;
			}
		}
		else if( arg == "--composition" )
		{
			if( ++i >= argc || !ParseSceneComposition( argv[i], options.composition ) )
			{
				return false;
			}
		}
		else if( arg == "--planet-layers" )
		{
			if( ++i >= argc || !ParsePlanetLayers( argv[i], options.planetLayers ) )
			{
				return false;
			}
		}
		else if( arg == "--planet-cloud-date" )
		{
			if( ++i >= argc || !ParsePlanetCloudDate( argv[i], options ) )
			{
				return false;
			}
		}
		else if( arg == "--sun-effects" )
		{
			if( ++i >= argc || !ParseSunEffects( argv[i], options.sunEffects ) )
			{
				return false;
			}
		}
		else if( arg == "--inspect-client-assets" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.inspectionReportPath = argv[i];
		}
		else
		{
			return false;
		}
	}

	if( options.inputPath.empty() )
	{
		options.inputPath = DefaultAssetPath( options );
	}
	if( !options.materialModeExplicit && options.asset == "astero" )
	{
		options.materialMode = MaterialMode::EveV5;
	}
	if( !options.localLightsExplicit && options.asset == "astero" && options.materialMode == MaterialMode::EveV5 )
	{
		options.localLights = LocalLights::Authored;
	}
	if( options.lightingView == LightingView::Direct || options.lightingView == LightingView::Sh ||
		options.lightingView == LightingView::Environment )
	{
		options.localLights = LocalLights::Off;
	}
	if( options.reflectionSource == ReflectionSource::Auto )
	{
		if( options.qualityRung < QualityRung::Scene || options.sceneFixture == SceneFixture::Empty )
		{
			options.resolvedReflectionSource = ReflectionSource::Off;
		}
		else if( options.sceneFixture == SceneFixture::Fitting )
		{
			options.resolvedReflectionSource = ReflectionSource::Static;
		}
		else
		{
			options.resolvedReflectionSource = ReflectionSource::Dynamic;
		}
	}
	else
	{
		options.resolvedReflectionSource = options.reflectionSource;
	}
	if( options.lightingView == LightingView::Direct || options.lightingView == LightingView::Sh ||
		options.lightingView == LightingView::Local )
	{
		options.resolvedReflectionSource = ReflectionSource::Off;
	}
	if( options.resolvedReflectionSource != ReflectionSource::Off &&
		( options.qualityRung < QualityRung::Scene || options.sceneFixture == SceneFixture::Empty ) )
	{
		std::cerr << "Static and dynamic reflections require a populated scene at the scene rung or higher\n";
		return false;
	}
	options.resolvedAttachments = options.attachments == Attachments::Auto ?
		( options.asset == "astero" && options.materialMode == MaterialMode::EveV5 &&
				  options.materialView == MaterialView::Lit && options.areaView == AreaView::All &&
				  options.qualityRung >= QualityRung::Model ?
			  Attachments::Authored :
			  Attachments::Off ) :
		options.attachments;
	if( options.resolvedAttachments == Attachments::Authored &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ) )
	{
		std::cerr << "Authored attachments require --asset astero --material-mode eve-v5\n";
		return false;
	}
	const bool compatibleLightingModel = options.asset == "astero" && options.materialMode == MaterialMode::EveV5 &&
		options.materialView == MaterialView::Lit && options.areaView == AreaView::All &&
		options.qualityRung >= QualityRung::Model;
	// The current macOS client disables dynamic-light shadows and ships no V5 receiver.
	options.resolvedLocalShadows =
		options.localShadows == LocalShadows::Auto ? LocalShadows::Off : options.localShadows;
	if( options.resolvedLocalShadows == LocalShadows::Authored &&
		( !compatibleLightingModel || options.localLights != LocalLights::Authored ) )
	{
		std::cerr << "Authored local shadows require authored local lights on a lit, all-area Astero using eve-v5\n";
		return false;
	}
	if( options.resolvedLocalShadows == LocalShadows::Validation &&
		( !compatibleLightingModel || options.localLights != LocalLights::Validation ) )
	{
		std::cerr
			<< "Validation local shadows require the validation local light on a lit, all-area Astero using eve-v5\n";
		return false;
	}
	options.resolvedDecals =
		options.decals == Decals::Auto ? ( compatibleLightingModel ? Decals::Authored : Decals::Off ) : options.decals;
	if( options.resolvedDecals == Decals::Authored && !compatibleLightingModel )
	{
		std::cerr << "Authored decals require a lit, all-area Astero using eve-v5 at the model rung or higher\n";
		return false;
	}
	const bool directSunEnabled =
		options.lightingView == LightingView::Combined || options.lightingView == LightingView::Direct;
	options.resolvedShadows = options.shadows == Shadows::Auto ?
		( compatibleLightingModel && directSunEnabled ? Shadows::High : Shadows::Off ) :
		options.shadows;
	options.resolvedAmbientOcclusion = options.ambientOcclusion == AmbientOcclusion::Auto ?
		( compatibleLightingModel ? AmbientOcclusion::High : AmbientOcclusion::Off ) :
		options.ambientOcclusion;
	if( options.resolvedShadows != Shadows::Off && ( !compatibleLightingModel || !directSunEnabled ) )
	{
		std::cerr
			<< "Directional shadows require a lit, all-area Astero using eve-v5 with direct or combined lighting\n";
		return false;
	}
	if( options.resolvedLocalShadows != LocalShadows::Off && options.resolvedShadows == Shadows::Low )
	{
		std::cerr << "Local shadows require high native quality and cannot be combined with low directional shadows\n";
		return false;
	}
	if( options.resolvedAmbientOcclusion != AmbientOcclusion::Off && !compatibleLightingModel )
	{
		std::cerr << "Ambient occlusion requires a lit, all-area Astero using eve-v5\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::BentNormal &&
		( options.resolvedAmbientOcclusion == AmbientOcclusion::Off || options.aoMethod != AoMethod::Cortao ) )
	{
		std::cerr << "Bent-normal capture requires enabled CORTAO\n";
		return false;
	}
	if( ( options.renderProduct == RenderProduct::Shadow || options.renderProduct == RenderProduct::ShadowAtlas ) &&
		options.resolvedShadows == Shadows::Off )
	{
		std::cerr << "Shadow render products require enabled directional shadows\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::LocalShadowAtlas && options.resolvedLocalShadows == LocalShadows::Off )
	{
		std::cerr << "The local-shadow-atlas render product requires enabled local shadows\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::AmbientOcclusion &&
		options.resolvedAmbientOcclusion == AmbientOcclusion::Off )
	{
		std::cerr << "AO render product requires enabled ambient occlusion\n";
		return false;
	}
	if( options.sunEffects == SunEffects::Auto )
	{
		options.resolvedSunEffects = options.sceneFixture == SceneFixture::NewEden ?
			( options.qualityRung >= QualityRung::HdrPost ? SunEffects::All : SunEffects::Flare ) :
			SunEffects::Off;
	}
	else
	{
		options.resolvedSunEffects = options.sunEffects;
	}
	if( options.sceneFixture != SceneFixture::NewEden && options.resolvedSunEffects != SunEffects::Off )
	{
		std::cerr << "Sun effects require --scene-fixture new-eden\n";
		return false;
	}
	if( ( options.resolvedSunEffects == SunEffects::GodRays || options.resolvedSunEffects == SunEffects::All ) &&
		options.qualityRung < QualityRung::HdrPost )
	{
		std::cerr << "God rays require --quality-rung hdr-post or hdr-exposure\n";
		return false;
	}
	options.resolvedDynamicExposure = options.dynamicExposure == DynamicExposure::Auto ?
		( options.qualityRung >= QualityRung::HdrExposure ? DynamicExposure::Client : DynamicExposure::Off ) :
		options.dynamicExposure;
	if( options.resolvedDynamicExposure == DynamicExposure::Client && options.qualityRung < QualityRung::HdrExposure )
	{
		std::cerr << "Client dynamic exposure requires --quality-rung hdr-exposure or hdr-finish\n";
		return false;
	}
	options.resolvedBloom = options.bloom == PostFinishMode::Auto ?
		( options.qualityRung == QualityRung::HdrFinish ? PostFinishMode::Client : PostFinishMode::Off ) :
		options.bloom;
	options.resolvedFilmGrain = options.filmGrain == PostFinishMode::Auto ?
		( options.qualityRung == QualityRung::HdrFinish ? PostFinishMode::Client : PostFinishMode::Off ) :
		options.filmGrain;
	if( ( options.resolvedBloom == PostFinishMode::Client || options.resolvedFilmGrain == PostFinishMode::Client ) &&
		options.qualityRung != QualityRung::HdrFinish )
	{
		std::cerr << "Client bloom and film grain require --quality-rung hdr-finish\n";
		return false;
	}
	const bool distortionCompatible = options.qualityRung == QualityRung::HdrFinish &&
		options.asset == "astero" && options.materialMode == MaterialMode::EveV5 &&
		options.shaderTier == ShaderTier::High &&
		( options.areaView == AreaView::All || options.areaView == AreaView::Distortion );
	options.resolvedDistortion = options.distortion == DistortionMode::Auto ?
		( distortionCompatible ? DistortionMode::Authored : DistortionMode::Off ) :
		options.distortion;
	if( options.resolvedDistortion == DistortionMode::Authored && !distortionCompatible )
	{
		std::cerr << "Authored distortion requires high-tier Astero eve-v5 rendering at hdr-finish\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::HdrComposite && options.qualityRung < QualityRung::HdrPost )
	{
		std::cerr << "The hdr-composite render product requires --quality-rung hdr-post or hdr-exposure\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::PostTonemap && options.qualityRung < QualityRung::HdrPost )
	{
		std::cerr << "The post-tonemap render product requires --quality-rung hdr-post or hdr-exposure\n";
		return false;
	}
	if( ( options.renderProduct == RenderProduct::Bloom || options.renderProduct == RenderProduct::FinalPostprocess ) &&
		options.qualityRung != QualityRung::HdrFinish )
	{
		std::cerr << "Bloom and final-postprocess products require --quality-rung hdr-finish\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::Distortion &&
		options.resolvedDistortion != DistortionMode::Authored )
	{
		std::cerr << "The distortion render product requires authored distortion\n";
		return false;
	}
	if( options.exposureSequence != ExposureSequence::None &&
		( options.resolvedDynamicExposure != DynamicExposure::Client ||
		  options.qualityRung < QualityRung::HdrExposure ) )
	{
		std::cerr << "Exposure camera sequences require client dynamic exposure at the hdr-exposure rung\n";
		return false;
	}
	if( options.exposureSequence != ExposureSequence::None &&
		( options.maxFrames <= 0 ||
		  static_cast<uint64_t>( options.maxFrames ) < static_cast<uint64_t>( options.exposureHold ) * 2 ) )
	{
		std::cerr << "Exposure camera sequences require at least two --exposure-hold phases\n";
		return false;
	}
	if( options.exposureSequence != ExposureSequence::None && options.capturePrefix.empty() )
	{
		std::cerr << "Exposure camera sequences require --capture-prefix for their CSV report\n";
		return false;
	}
	if( options.validateExposureTone &&
		( options.qualityRung < QualityRung::HdrExposure ||
		  options.resolvedDynamicExposure != DynamicExposure::Client ) )
	{
		std::cerr << "--validate-exposure-tone requires hdr-exposure or hdr-finish with client exposure\n";
		return false;
	}
	if( options.validateExposureTone && ( options.maxFrames <= 0 || options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-exposure-tone requires finite frames and --capture-prefix\n";
		return false;
	}
	if( options.validateExposureTone && static_cast<uint32_t>( options.maxFrames ) < options.exposureHold )
	{
		std::cerr << "--validate-exposure-tone requires at least --exposure-hold frames for settling\n";
		return false;
	}
	if( options.validateExposureTone && options.framePacingCheck )
	{
		std::cerr << "Run frame pacing separately from CPU-synchronized exposure diagnostics\n";
		return false;
	}
	if( options.validatePostFinish &&
		( options.qualityRung != QualityRung::HdrFinish || options.resolvedBloom != PostFinishMode::Client ||
		  options.resolvedFilmGrain != PostFinishMode::Client ) )
	{
		std::cerr << "--validate-post-finish requires hdr-finish with client bloom and film grain\n";
		return false;
	}
	if( options.validatePostFinish && ( options.maxFrames <= 0 || options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-post-finish requires finite frames and --capture-prefix\n";
		return false;
	}
	if( options.framePacingCheck &&
		( options.maxFrames <= 0 || static_cast<uint32_t>( options.maxFrames ) <= options.timingWarmup ) )
	{
		std::cerr << "--frame-pacing-check requires a finite --frames count greater than --timing-warmup\n";
		return false;
	}
	if( options.validateComposition && options.maxFrames >= 0 &&
		static_cast<uint32_t>( options.maxFrames ) < std::max( options.timingWarmup, 2u ) )
	{
		std::cerr << "--validate-composition requires enough finite frames to reach the composition warm-up gate\n";
		return false;
	}
	if( options.validateComposition && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.validateExposureTone && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.validatePostFinish && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.validateDistortion &&
		( options.resolvedDistortion != DistortionMode::Authored || options.maxFrames <= 0 ||
		  options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-distortion requires authored distortion, finite frames, and --capture-prefix\n";
		return false;
	}
	if( options.validateDistortion && static_cast<uint32_t>( options.maxFrames ) < options.exposureHold )
	{
		std::cerr << "--validate-distortion requires at least --exposure-hold frames for settled final-color validation\n";
		return false;
	}
	if( options.validateDistortion && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.renderProduct != RenderProduct::Window &&
		( options.capturePrefix.empty() || options.maxFrames <= 0 || options.qualityRung == QualityRung::Shell ) )
	{
		return false;
	}

	return true;
}

uint32_t GetBackingDimension( CGFloat points, CGFloat backingScale )
{
	return std::max( 1u, static_cast<uint32_t>( std::llround( points * backingScale ) ) );
}

}

@interface EveSceneProbeContentView : NSView
@end

@implementation EveSceneProbeContentView
- (BOOL)canBecomeKeyView
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}
@end

namespace
{

NSWindow* CreateWindow( const Options& options, NSView** outView )
{
	NSRect frame = NSMakeRect( 0, 0, options.windowWidth, options.windowHeight );
	NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

	if( !options.windowed )
	{
		NSScreen* screen = [NSScreen mainScreen];
		frame = screen ? [screen frame] : frame;
		styleMask = NSWindowStyleMaskBorderless;
	}

	NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
												   styleMask:styleMask
													 backing:NSBackingStoreBuffered
													   defer:NO];
	[window setReleasedWhenClosed:NO];
	[window setTitle:@"TrinityAL EVE Scene Probe"];
	[window setBackgroundColor:[NSColor blackColor]];
	[window setOpaque:YES];

	if( options.windowed )
	{
		[window center];
	}
	else
	{
		[window setFrame:frame display:YES];
		[window setLevel:NSMainMenuWindowLevel + 1];
		[window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
				NSWindowCollectionBehaviorFullScreenPrimary];
	}

	EveSceneProbeContentView* view = [[EveSceneProbeContentView alloc] initWithFrame:frame];
	view.wantsLayer = YES;
	view.layer = [CAMetalLayer layer];
	[window setContentView:view];
	[window makeFirstResponder:view];
	*outView = view;
	return window;
}

void ProcessEvents( NSWindow* window )
{
	while( true )
	{
		NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
											untilDate:[NSDate distantPast]
											   inMode:NSDefaultRunLoopMode
											  dequeue:YES];
		if( event == nil )
		{
			break;
		}

		if( event.type == NSEventTypeKeyDown && event.keyCode == 53 )
		{
			g_shouldQuit = true;
			[window close];
			continue;
		}

		[NSApp sendEvent:event];
	}
}

bool EnsureParentDirectory( const std::string& path )
{
	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	NSString* parent = [nsPath stringByDeletingLastPathComponent];
	if( parent.length == 0 )
	{
		return true;
	}

	NSError* error = nil;
	if( ![[NSFileManager defaultManager] createDirectoryAtPath:parent
								   withIntermediateDirectories:YES
													attributes:nil
														 error:&error] )
	{
		std::cerr << "Failed to create capture directory: " << error.localizedDescription.UTF8String << "\n";
		return false;
	}
	return true;
}

bool CaptureWindowPng( NSWindow* window, const std::string& path )
{
	if( !EnsureParentDirectory( path ) )
	{
		return false;
	}

	CGImageRef image = CGWindowListCreateImage( CGRectNull,
												kCGWindowListOptionIncludingWindow,
												static_cast<CGWindowID>( window.windowNumber ),
												kCGWindowImageBoundsIgnoreFraming );
	if( !image )
	{
		std::cerr << "Failed to capture window image\n";
		return false;
	}

	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	NSURL* url = [NSURL fileURLWithPath:nsPath];
	CGImageDestinationRef destination = CGImageDestinationCreateWithURL(
		(__bridge CFURLRef)url, (__bridge CFStringRef)UTTypePNG.identifier, 1, nullptr );
	if( !destination )
	{
		CGImageRelease( image );
		std::cerr << "Failed to create PNG destination\n";
		return false;
	}

	CGImageDestinationAddImage( destination, image, nullptr );
	const bool finalized = CGImageDestinationFinalize( destination );
	CFRelease( destination );
	CGImageRelease( image );

	if( !finalized )
	{
		std::cerr << "Failed to finalize PNG capture\n";
		return false;
	}
	return true;
}

bool CaptureProbeProductPng( void* probe, const std::string& path )
{
	if( !EnsureParentDirectory( path ) )
	{
		return false;
	}
	const uint8_t* pixels = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t pitch = 0;
	if( !TrinityStandaloneProbeGetCapturedProduct( probe, &pixels, &width, &height, &pitch ) )
	{
		std::cerr << "No synchronized render-product readback is available\n";
		return false;
	}
	CGDataProviderRef provider =
		CGDataProviderCreateWithData( nullptr, pixels, static_cast<size_t>( pitch ) * height, nullptr );
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGImageRef image = CGImageCreate( width,
									  height,
									  8,
									  32,
									  pitch,
									  colorSpace,
									  kCGBitmapByteOrderDefault | kCGImageAlphaLast,
									  provider,
									  nullptr,
									  false,
									  kCGRenderingIntentDefault );
	CGColorSpaceRelease( colorSpace );
	CGDataProviderRelease( provider );
	if( !image )
	{
		return false;
	}
	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	CGImageDestinationRef destination = CGImageDestinationCreateWithURL(
		(__bridge CFURLRef)[NSURL fileURLWithPath:nsPath], (__bridge CFStringRef)UTTypePNG.identifier, 1, nullptr );
	if( !destination )
	{
		CGImageRelease( image );
		return false;
	}
	CGImageDestinationAddImage( destination, image, nullptr );
	const bool finalized = CGImageDestinationFinalize( destination );
	CFRelease( destination );
	CGImageRelease( image );
	return finalized;
}

std::string CaptureBasePath( const Options& options )
{
	const std::string materialSuffix = "_" + MaterialModeName( options.materialMode ) +
		( options.materialView == MaterialView::Lit ? "" : "_" + MaterialViewName( options.materialView ) ) +
		( options.areaView == AreaView::All ? "" : "_area-" + AreaViewName( options.areaView ) ) + "_lit-" +
		LightingViewName( options.lightingView ) + "_sh-" + ShSourceName( options.shSource ) + "_ll-" +
		LocalLightsName( options.localLights ) + "_lsh-" + LocalShadowsName( options.resolvedLocalShadows ) +
		"_reflsrc-" + ReflectionSourceName( options.resolvedReflectionSource ) + "_reflcorr-" +
		ReflectionCorrectionName( options.reflectionCorrection ) + "_nrm-" +
		NormalMapModeName( options.normalMapMode ) + "_cam-" + CameraViewName( options.cameraView ) + "_comp-" +
		SceneCompositionName( options.composition ) + "_pl-" + PlanetLayersName( options.planetLayers ) + "_date-" +
		PlanetCloudDateName( options );
	const std::string sunSuffix = "_sun-" + SunEffectsName( options.resolvedSunEffects ) + "_att-" +
		AttachmentsName( options.resolvedAttachments ) + "-" + AttachmentViewName( options.attachmentView ) +
		"_decal-" + DecalsName( options.resolvedDecals ) + "-" + DecalViewName( options.decalView ) + "-kills-" +
		std::to_string( options.killCount ) + "_shadow-" + ShadowsName( options.resolvedShadows ) + "_ao-" +
		AmbientOcclusionName( options.resolvedAmbientOcclusion ) +
		( options.resolvedAmbientOcclusion == AmbientOcclusion::Off ? "" : "-" + AoMethodName( options.aoMethod ) );
	const std::string fullPath = options.capturePrefix + "_" + options.asset + "_" +
		QualityRungName( options.qualityRung ) + materialSuffix + sunSuffix + "_bloom-" +
		PostFinishModeName( options.resolvedBloom ) + "_grain-" + PostFinishModeName( options.resolvedFilmGrain ) +
		"_dist-" + DistortionModeName( options.resolvedDistortion );
	const size_t filenameOffset = fullPath.find_last_of( "/\\" );
	const size_t filenameLength = fullPath.size() - ( filenameOffset == std::string::npos ? 0 : filenameOffset + 1 );
	if( filenameLength <= 220 )
	{
		return fullPath;
	}

	uint64_t hash = 1469598103934665603ull;
	for( unsigned char value : fullPath )
	{
		hash ^= value;
		hash *= 1099511628211ull;
	}
	char hashText[17];
	std::snprintf( hashText, sizeof( hashText ), "%016llx", static_cast<unsigned long long>( hash ) );
	return options.capturePrefix + "_" + options.asset + "_" + QualityRungName( options.qualityRung ) + "_decal-" +
		DecalsName( options.resolvedDecals ) + "-" + DecalViewName( options.decalView ) + "-kills-" +
		std::to_string( options.killCount ) + "_pl-" + PlanetLayersName( options.planetLayers ) + "_date-" +
		PlanetCloudDateName( options ) + "_" + hashText;
}

bool WriteCaptureMetadata( const Options& options,
						   uint32_t width,
						   uint32_t height,
						   int renderedFrames,
						   const std::vector<std::pair<std::string, std::string>>& productStats )
{
	if( options.capturePrefix.empty() )
	{
		return true;
	}

	const std::string metadataPath = CaptureBasePath( options ) + ".txt";
	if( !EnsureParentDirectory( metadataPath ) )
	{
		return false;
	}

	std::ofstream metadata( metadataPath.c_str() );
	if( !metadata )
	{
		std::cerr << "Failed to write capture metadata: " << metadataPath << "\n";
		return false;
	}

	metadata << "asset=" << options.asset << "\n";
	metadata << "input=" << options.inputPath << "\n";
	metadata << "qualityRung=" << QualityRungName( options.qualityRung ) << "\n";
	metadata << "materialView=" << MaterialViewName( options.materialView ) << "\n";
	metadata << "materialMode=" << MaterialModeName( options.materialMode ) << "\n";
	metadata << "areaView=" << AreaViewName( options.areaView ) << "\n";
	metadata << "lightingView=" << LightingViewName( options.lightingView ) << "\n";
	metadata << "shSource=" << ShSourceName( options.shSource ) << "\n";
	metadata << "localLights=" << LocalLightsName( options.localLights ) << "\n";
	metadata << "localShadowsRequested=" << LocalShadowsName( options.localShadows ) << "\n";
	metadata << "localShadowsResolved=" << LocalShadowsName( options.resolvedLocalShadows ) << "\n";
	metadata << "localShadowEligibility="
			 << ( options.resolvedLocalShadows == LocalShadows::Authored ?
					  "probe-all-active" :
					  ( options.resolvedLocalShadows == LocalShadows::Validation ? "probe-validation" : "none" ) )
			 << "\n";
	metadata << "nativeShadowQuality="
			 << ( options.resolvedLocalShadows != LocalShadows::Off ? "high" : ShadowsName( options.resolvedShadows ) )
			 << "\n";
	metadata << "localShadowReceiver=probe-raster-atlas-to-r16-mask\n";
	metadata << "localShadowReceiverConsumption=current-v5-main-readnone\n";
	metadata << "attachmentsRequested=" << AttachmentsName( options.attachments ) << "\n";
	metadata << "attachmentsResolved=" << AttachmentsName( options.resolvedAttachments ) << "\n";
	metadata << "attachmentView=" << AttachmentViewName( options.attachmentView ) << "\n";
	metadata << "decalsRequested=" << DecalsName( options.decals ) << "\n";
	metadata << "decalsResolved=" << DecalsName( options.resolvedDecals ) << "\n";
	metadata << "decalView=" << DecalViewName( options.decalView ) << "\n";
	metadata << "killCount=" << options.killCount << "\n";
	metadata << "modelYawDegrees=" << options.modelYawDegrees << "\n";
	metadata << "shaderTier=" << ShaderTierName( options.shaderTier ) << "\n";
	metadata << "reflectionSourceRequested=" << ReflectionSourceName( options.reflectionSource ) << "\n";
	metadata << "reflectionSourceResolved=" << ReflectionSourceName( options.resolvedReflectionSource ) << "\n";
	metadata << "reflectionCorrection=" << ReflectionCorrectionName( options.reflectionCorrection ) << "\n";
	metadata << "normalMap=" << NormalMapModeName( options.normalMapMode ) << "\n";
	metadata << "shadowsRequested=" << ShadowsName( options.shadows ) << "\n";
	metadata << "shadowsResolved=" << ShadowsName( options.resolvedShadows ) << "\n";
	metadata << "aoRequested=" << AmbientOcclusionName( options.ambientOcclusion ) << "\n";
	metadata << "aoResolved=" << AmbientOcclusionName( options.resolvedAmbientOcclusion ) << "\n";
	metadata << "aoMethod=" << AoMethodName( options.aoMethod ) << "\n";
	metadata << "renderProduct=" << RenderProductName( options.renderProduct ) << "\n";
	metadata << "cameraView=" << CameraViewName( options.cameraView ) << "\n";
	metadata << "composition=" << SceneCompositionName( options.composition ) << "\n";
	metadata << "planetLayers=" << PlanetLayersName( options.planetLayers ) << "\n";
	metadata << "planetCloudDate=" << PlanetCloudDateName( options ) << "\n";
	metadata << "sunEffectsRequested=" << SunEffectsName( options.sunEffects ) << "\n";
	metadata << "sunEffectsResolved=" << SunEffectsName( options.resolvedSunEffects ) << "\n";
	metadata << "backgroundCapture=" << ( options.backgroundCapture ? "true" : "false" ) << "\n";
	metadata << "validateComposition=" << ( options.validateComposition ? "true" : "false" ) << "\n";
	metadata << "dynamicExposureRequested=" << DynamicExposureName( options.dynamicExposure ) << "\n";
	metadata << "dynamicExposureResolved=" << DynamicExposureName( options.resolvedDynamicExposure ) << "\n";
	metadata << "validateExposureTone=" << ( options.validateExposureTone ? "true" : "false" ) << "\n";
	metadata << "bloomRequested=" << PostFinishModeName( options.bloom ) << "\n";
	metadata << "bloomResolved=" << PostFinishModeName( options.resolvedBloom ) << "\n";
	metadata << "bloomImplementation=legacy-highpass-blur\n";
	metadata << "filmGrainRequested=" << PostFinishModeName( options.filmGrain ) << "\n";
	metadata << "filmGrainResolved=" << PostFinishModeName( options.resolvedFilmGrain ) << "\n";
	metadata << "validatePostFinish=" << ( options.validatePostFinish ? "true" : "false" ) << "\n";
	metadata << "distortionRequested=" << DistortionModeName( options.distortion ) << "\n";
	metadata << "distortionResolved=" << DistortionModeName( options.resolvedDistortion ) << "\n";
	metadata << "distortionClientPolicy=high-shader-tier\n";
	metadata << "validateDistortion=" << ( options.validateDistortion ? "true" : "false" ) << "\n";
	metadata << "exposureSequence=" << ExposureSequenceName( options.exposureSequence ) << "\n";
	metadata << "exposureHold=" << options.exposureHold << "\n";
	metadata << "framePacingCheck=" << ( options.framePacingCheck ? "true" : "false" ) << "\n";
	metadata << "timingWarmup=" << options.timingWarmup << "\n";
	metadata << "renderWidth=" << width << "\n";
	metadata << "renderHeight=" << height << "\n";
	metadata << "frames=" << renderedFrames << "\n";
	for( const auto& stats : productStats )
	{
		metadata << stats.first << "Stats=" << stats.second << "\n";
	}
	return true;
}

bool AppendHdrCompositeStats( void* probe, std::vector<std::pair<std::string, std::string>>& productStats )
{
	uint32_t format = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint64_t rawHash = 0;
	uint64_t finiteComponents = 0;
	uint64_t nanComponents = 0;
	uint64_t infComponents = 0;
	uint64_t negativeComponents = 0;
	uint64_t saturatedComponents = 0;
	uint64_t pixelsAboveOne = 0;
	double minimumLuminance = 0.0;
	double meanLuminance = 0.0;
	double maximumLuminance = 0.0;
	double p50Luminance = 0.0;
	double p95Luminance = 0.0;
	double p99Luminance = 0.0;
	bool validationPassed = false;
	if( !TrinityStandaloneProbeGetHdrCompositeDiagnostics( probe,
														   &format,
														   &width,
														   &height,
														   &rawHash,
														   &finiteComponents,
														   &nanComponents,
														   &infComponents,
														   &negativeComponents,
														   &saturatedComponents,
														   &pixelsAboveOne,
														   &minimumLuminance,
														   &meanLuminance,
														   &maximumLuminance,
														   &p50Luminance,
														   &p95Luminance,
														   &p99Luminance,
														   &validationPassed ) )
	{
		std::cerr << "No raw FP16 composite diagnostics are available\n";
		return false;
	}

	std::ostringstream stats;
	stats << "source=PreTonemapColor format=R16G16B16A16_FLOAT formatValue=" << format << " dimensions=" << width << "x"
		  << height << " rawHash=" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << rawHash << std::dec
		  << " finite=" << finiteComponents << " nan=" << nanComponents << " inf=" << infComponents
		  << " negative=" << negativeComponents << " saturated=" << saturatedComponents
		  << " pixelsAboveOne=" << pixelsAboveOne << std::setprecision( 10 ) << " luminanceMin=" << minimumLuminance
		  << " luminanceMean=" << meanLuminance << " luminanceMax=" << maximumLuminance
		  << " luminanceP50=" << p50Luminance << " luminanceP95=" << p95Luminance << " luminanceP99=" << p99Luminance
		  << " validation=" << ( validationPassed ? "pass" : "fail" );
	productStats.emplace_back( "hdrCompositeRaw", stats.str() );
	return validationPassed;
}

bool EvaluateFramePacing( const std::vector<double>& samples, std::string& result )
{
	if( samples.empty() )
	{
		result = "samples=0 validation=fail";
		std::cerr << "Frame-pacing validation has no post-warm-up samples\n";
		return false;
	}
	std::vector<double> sorted = samples;
	std::sort( sorted.begin(), sorted.end() );
	auto percentile = [&]( double fraction ) {
		const size_t index = static_cast<size_t>( std::ceil( fraction * sorted.size() ) ) - 1;
		return sorted[std::min( index, sorted.size() - 1 )];
	};
	const double median = percentile( 0.50 );
	const double p95 = percentile( 0.95 );
	const double p99 = percentile( 0.99 );
	const double maximum = sorted.back();
	double sum = 0.0;
	uint64_t exceedingTwiceMedian = 0;
	for( double sample : samples )
	{
		sum += sample;
		if( sample > median * 2.0 )
		{
			++exceedingTwiceMedian;
		}
	}
	const double mean = sum / static_cast<double>( samples.size() );
	const double throughput = mean > 0.0 ? 1000.0 / mean : 0.0;
	const double outlierFraction = static_cast<double>( exceedingTwiceMedian ) / samples.size();
	const bool passed = median > 0.0 && p95 <= median * 1.75 && p99 <= median * 3.0 && maximum <= median * 5.0 &&
		outlierFraction <= 0.02;

	std::ostringstream stats;
	stats << std::fixed << std::setprecision( 4 ) << "samples=" << samples.size() << " medianMs=" << median
		  << " p95Ms=" << p95 << " p99Ms=" << p99 << " maximumMs=" << maximum << " meanThroughputFps=" << throughput
		  << " overTwiceMedian=" << exceedingTwiceMedian << " overTwiceMedianPercent=" << outlierFraction * 100.0
		  << " validation=" << ( passed ? "pass" : "fail" );
	result = stats.str();
	std::cerr << "EVE RC-09 frame pacing: " << result << "\n";
	return passed;
}

double ExposureTarget( const TrinityStandalonePostProcessDiagnostics& sample )
{
	const double luminance = std::max( 0.0001, 0.5 * ( sample.exposure[5] + sample.exposure[6] ) );
	const double compensation = std::clamp(
		0.5 / luminance,
		std::exp2( static_cast<double>( sample.minExposure ) ),
		std::exp2( static_cast<double>( sample.maxExposure ) ) );
	return 0.5 / compensation;
}

double Median( std::vector<double> values )
{
	if( values.empty() )
	{
		return 0.0;
	}
	std::sort( values.begin(), values.end() );
	return values[values.size() / 2];
}

bool EvaluateExposureSamples(
	const Options& options,
	const std::vector<TrinityStandalonePostProcessDiagnostics>& samples,
	std::string& summary )
{
	uint64_t invalidFrames = 0;
	uint64_t histogramMismatches = 0;
	uint64_t recurrenceMismatches = 0;
	uint64_t overshoots = 0;
	double maximumRecurrenceError = 0.0;
	std::vector<double> increaseRates;
	std::vector<double> decreaseRates;
	auto near = []( float actual, float expected ) { return std::abs( actual - expected ) <= 0.000001f; };
	for( const auto& sample : samples )
	{
		const uint64_t histogramTotal = std::accumulate(
			std::begin( sample.histogram ), std::begin( sample.histogram ) + 64, uint64_t{ 0 } );
		const uint64_t expectedPixels = static_cast<uint64_t>( sample.sourceWidth ) * sample.sourceHeight;
		if( histogramTotal != expectedPixels )
		{
			++histogramMismatches;
		}
		const bool settingsValid = sample.tonemappingMethod == 0 && near( sample.minBrightness, 0.9f ) &&
			near( sample.maxBrightness, 0.98f ) && near( sample.increaseSpeed, 2.0f ) &&
			near( sample.decreaseSpeed, 1.5f ) && near( sample.minLuminance, 0.4649f ) &&
			near( sample.maxLuminance, 10.0f ) && near( sample.exposureInfluence, 1.0f ) &&
			near( sample.exposureMiddleValue, 0.55f ) && near( sample.exposureAdjustment, 0.0f ) &&
			near( sample.minExposure, -3.7f ) && near( sample.maxExposure, 10.0f ) &&
			near( sample.shoulderStrength, 0.125f ) && near( sample.linearStrength, 0.25f ) &&
			near( sample.linearAngle, 0.1f ) && near( sample.toeStrength, 0.15f ) &&
			near( sample.toeNumerator, 0.021f ) && near( sample.toeDenominator, 0.3f ) &&
			near( sample.whiteScale, 2.5f ) && near( sample.outputGamma, 1.0f );
		const bool stateValid = std::isfinite( sample.exposure[0] ) && sample.exposure[0] >= 0.0f &&
			std::isfinite( sample.exposure[1] ) && std::isfinite( sample.exposure[3] ) &&
			std::isfinite( sample.exposure[4] ) && std::isfinite( sample.exposure[5] ) &&
			std::isfinite( sample.exposure[6] ) && sample.exposure[3] <= sample.exposure[4] &&
			sample.exposure[5] <= sample.exposure[6];
		if( !sample.dynamicExposureActive || !sample.histogramCreated || !sample.histogramMerged ||
			!sample.exposureMeasured || !sample.tonemappingSucceeded || !settingsValid || !stateValid )
		{
			++invalidFrames;
		}
	}

	for( size_t index = 1; index < samples.size(); ++index )
	{
		const auto& previous = samples[index - 1];
		const auto& current = samples[index];
		const double deltaTime = current.exposure[1] - previous.exposure[1];
		if( deltaTime <= 0.0 || previous.exposure[0] <= 0.0f )
		{
			continue;
		}
		const double target = ExposureTarget( current );
		const double speed = target > previous.exposure[0] ? current.increaseSpeed : current.decreaseSpeed;
		const double alpha = std::clamp( 1.0 - std::exp( -deltaTime * speed ), 0.0, 1.0 );
		const double expectedRoot = std::sqrt( previous.exposure[0] ) +
			( std::sqrt( target ) - std::sqrt( previous.exposure[0] ) ) * alpha;
		const double expected = expectedRoot * expectedRoot;
		const double error = std::abs( current.exposure[0] - expected );
		maximumRecurrenceError = std::max( maximumRecurrenceError, error );
		const double tolerance = std::max( 0.0001, std::abs( expected ) * 0.001 );
		if( error > tolerance )
		{
			++recurrenceMismatches;
		}
		const double low = std::min( static_cast<double>( previous.exposure[0] ), target ) - tolerance;
		const double high = std::max( static_cast<double>( previous.exposure[0] ), target ) + tolerance;
		if( current.exposure[0] < low || current.exposure[0] > high )
		{
			++overshoots;
		}
		const double denominator = std::sqrt( target ) - std::sqrt( previous.exposure[0] );
		if( std::abs( denominator ) > 0.000001 )
		{
			const double observedAlpha = std::clamp(
				( std::sqrt( current.exposure[0] ) - std::sqrt( previous.exposure[0] ) ) / denominator,
				0.0,
				0.999999 );
			const double rate = -std::log( 1.0 - observedAlpha ) / deltaTime;
			( target > previous.exposure[0] ? increaseRates : decreaseRates ).push_back( rate );
		}
	}

	bool sequencePassed = true;
	double stepFraction = 0.0;
	double terminalTrackingError = 0.0;
	double fittedRate = 0.0;
	if( options.exposureSequence != ExposureSequence::None )
	{
		const size_t hold = options.exposureHold;
		const size_t window = std::min<size_t>( 30, hold );
		std::vector<double> firstTargets;
		std::vector<double> secondTargets;
		for( size_t index = hold - window; index < hold && index < samples.size(); ++index )
			firstTargets.push_back( ExposureTarget( samples[index] ) );
		for( size_t index = std::min( samples.size(), hold * 2 ) - window;
			 index < std::min( samples.size(), hold * 2 ); ++index )
			secondTargets.push_back( ExposureTarget( samples[index] ) );
		const double firstTarget = Median( firstTargets );
		const double secondTarget = Median( secondTargets );
		stepFraction = firstTarget > 0.0 ? std::abs( secondTarget - firstTarget ) / firstTarget : 0.0;
		terminalTrackingError = secondTarget > 0.0 ?
			std::abs( samples[std::min( samples.size(), hold * 2 ) - 1].exposure[0] - secondTarget ) / secondTarget :
			1.0;
		const bool increasing = secondTarget > firstTarget;
		fittedRate = Median( increasing ? increaseRates : decreaseRates );
		const double expectedRate = increasing ? 2.0 : 1.5;
		sequencePassed = stepFraction >= 0.20 && terminalTrackingError <= 0.05 && fittedRate > 0.0 &&
			std::abs( fittedRate - expectedRate ) / expectedRate <= 0.05;
	}

	const bool passed = !samples.empty() && invalidFrames == 0 && histogramMismatches == 0 &&
		recurrenceMismatches == 0 && overshoots == 0 && sequencePassed;
	std::ostringstream out;
	out << std::fixed << std::setprecision( 8 ) << "samples=" << samples.size()
		<< " invalidFrames=" << invalidFrames << " histogramMismatches=" << histogramMismatches
		<< " recurrenceMismatches=" << recurrenceMismatches << " maximumRecurrenceError=" << maximumRecurrenceError
		<< " overshoots=" << overshoots << " increaseRate=" << Median( increaseRates )
		<< " decreaseRate=" << Median( decreaseRates ) << " stepFraction=" << stepFraction
		<< " fittedSequenceRate=" << fittedRate << " terminalTrackingError=" << terminalTrackingError
		<< " validation=" << ( passed ? "pass" : "fail" );
	summary = out.str();
	std::cerr << "EVE RC-10 exposure validation: " << summary << "\n";
	return passed;
}

bool WriteExposureCsv(
	const std::string& path,
	const std::vector<TrinityStandalonePostProcessDiagnostics>& samples )
{
	if( !EnsureParentDirectory( path ) )
	{
		return false;
	}
	std::ofstream output( path );
	if( !output )
	{
		return false;
	}
	output << "frame,realTime,simTime,adaptedLuminance,sceneTime,reserved2,lowerBin,upperBin,lowerLuminance,upperLuminance,reserved7";
	for( uint32_t bin = 0; bin < 65; ++bin )
		output << ",histogram" << bin;
	output << "\n";
	for( size_t frame = 0; frame < samples.size(); ++frame )
	{
		const auto& sample = samples[frame];
		output << frame << ',' << sample.realTime << ',' << sample.simTime;
		for( uint32_t value = 0; value < 8; ++value )
			output << ',' << std::setprecision( 10 ) << sample.exposure[value];
		for( uint32_t bin : sample.histogram )
			output << ',' << bin;
		output << '\n';
	}
	return output.good();
}

bool WriteToneContractJson(
	const Options& options,
	const TrinityStandaloneToneValidation& tone,
	const std::string& exposureSummary,
	DynamicExposure mode,
	const char* suffix )
{
	const std::string outputPath = CaptureBasePath( options ) + suffix;
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/PostProcessResources.json";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	if( !manifestInput || manifest.str().empty() )
	{
		std::cerr << "RC-10 could not read the postprocess resource manifest: " << manifestPath << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	output << "{\n"
		   << "  \"profile\": \"RC-10\",\n"
		   << "  \"toneMethod\": \"Uncharted2\",\n"
		   << "  \"containerMethod\": \"baked\",\n"
		   << "  \"acesActive\": false,\n"
		   << "  \"outputGamma\": 1.0,\n"
		   << "  \"dynamicExposure\": \"" << DynamicExposureName( mode ) << "\",\n"
		   << "  \"exposureSequence\": \"" << ExposureSequenceName( options.exposureSequence ) << "\",\n"
		   << "  \"preTonemapHash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' )
		   << tone.preTonemapHash << "\",\n"
		   << "  \"postTonemapHash\": \"" << std::setw( 16 ) << tone.postTonemapHash << std::dec << "\",\n"
		   << "  \"dimensions\": [" << tone.width << ", " << tone.height << "],\n"
		   << "  \"fullyBlackPixels\": " << tone.fullyBlackPixels << ",\n"
		   << "  \"fullyWhitePixels\": " << tone.fullyWhitePixels << ",\n"
		   << "  \"anySaturatedChannelPixels\": " << tone.anySaturatedChannelPixels << ",\n"
		   << "  \"luminance\": {\"p01\": " << tone.luminanceP01 << ", \"p50\": "
		   << tone.luminanceP50 << ", \"p99\": " << tone.luminanceP99 << "},\n"
		   << "  \"cpuReferenceError\": {\"mean\": " << tone.meanAbsoluteError << ", \"p999\": "
		   << tone.p999AbsoluteError << ", \"maximum\": " << tone.maximumAbsoluteError << "},\n"
		   << "  \"exposureValidation\": \"" << exposureSummary << "\",\n"
		   << "  \"validationPassed\": " << ( tone.valid ? "true" : "false" ) << ",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Tone contract report: " << outputPath << "\n";
	return output.good();
}

bool CompareToneValidations(
	const TrinityStandaloneToneValidation& off,
	const TrinityStandaloneToneValidation& client,
	std::string& summary )
{
	const uint64_t pixelCount = static_cast<uint64_t>( client.width ) * client.height;
	const bool sameInput = off.preTonemapHash == client.preTonemapHash;
	const bool distinctOutput = off.postTonemapHash != client.postTonemapHash;
	const bool reducedWhite = client.fullyWhitePixels < off.fullyWhitePixels;
	const bool clippingValid = client.fullyWhitePixels <= pixelCount / 1000 &&
		client.anySaturatedChannelPixels <= pixelCount * 3 / 100 &&
		client.fullyBlackPixels <= pixelCount / 1000;
	const bool lowEndRetained = client.luminanceP01 >= off.luminanceP01 * 0.75;
	const bool passed = off.valid && client.valid && sameInput && distinctOutput && reducedWhite &&
		clippingValid && lowEndRetained;
	std::ostringstream out;
	out << "sameInput=" << ( sameInput ? "true" : "false" )
		<< " distinctOutput=" << ( distinctOutput ? "true" : "false" )
		<< " offWhite=" << off.fullyWhitePixels << " clientWhite=" << client.fullyWhitePixels
		<< " clientAnySaturated=" << client.anySaturatedChannelPixels
		<< " offP01=" << off.luminanceP01 << " clientP01=" << client.luminanceP01
		<< " validation=" << ( passed ? "pass" : "fail" );
	summary = out.str();
	std::cerr << "EVE RC-10 matched tone comparison: " << summary << "\n";
	return passed;
}

bool WritePostFinishContractJson( const Options& options, const TrinityStandalonePostFinishValidation& validation )
{
	const std::string outputPath = CaptureBasePath( options ) + "_post-finish-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/PostFinishResources.json";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	if( !manifestInput || manifest.str().empty() )
	{
		std::cerr << "RC-11 could not read the post-finish resource manifest: " << manifestPath << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	output << "{\n"
		   << "  \"profile\": \"RC-11\",\n"
		   << "  \"bloomImplementation\": \"legacy-highpass-blur\",\n"
		   << "  \"newBloom\": false,\n"
		   << "  \"bloomHash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << validation.bloomHash
		   << "\",\n"
		   << "  \"postTonemapHash\": \"" << std::setw( 16 ) << validation.postTonemapHash << "\",\n"
		   << "  \"finalHash\": \"" << std::setw( 16 ) << validation.finalHash << std::dec << "\",\n"
		   << "  \"bloomDimensions\": [" << validation.bloomWidth << ", " << validation.bloomHeight << "],\n"
		   << "  \"finalDimensions\": [" << validation.finalWidth << ", " << validation.finalHeight << "],\n"
		   << "  \"bloomNonzeroPixels\": " << validation.bloomNonzeroPixels << ",\n"
		   << "  \"bloomInvalidComponents\": " << validation.bloomInvalidComponents << ",\n"
		   << "  \"bloomLuminance\": {\"minimum\": " << validation.bloomMinimumLuminance
		   << ", \"mean\": " << validation.bloomMeanLuminance << ", \"maximum\": " << validation.bloomMaximumLuminance
		   << "},\n"
		   << "  \"grainChangedPixels\": " << validation.grainChangedPixels << ",\n"
		   << "  \"grainAlphaChangedPixels\": " << validation.grainAlphaChangedPixels << ",\n"
		   << "  \"grainResidual\": {\"mean\": " << validation.grainMeanAbsoluteError
		   << ", \"p99\": " << validation.grainP99AbsoluteError
		   << ", \"maximum\": " << validation.grainMaximumAbsoluteError << "},\n"
		   << "  \"validationPassed\": " << ( validation.valid ? "true" : "false" ) << ",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Post-finish contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteDistortionContractJson(
	const Options& options,
	const TrinityStandaloneDistortionDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_distortion-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/AsteroDistortionResources.json";
	const std::string generatedCmfHashPath =
		ExecutableDirectory() + "/../Reports/AsteroDistortionGeneratedCmf.sha256";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	std::ifstream generatedCmfHashInput( generatedCmfHashPath );
	std::string generatedCmfHash;
	std::getline( generatedCmfHashInput, generatedCmfHash );
	if( !manifestInput || manifest.str().empty() || !generatedCmfHashInput || generatedCmfHash.empty() )
	{
		std::cerr << "RC-12A could not read distortion provenance reports: " << manifestPath << ", "
				  << generatedCmfHashPath << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	output << "{\n"
		   << "  \"profile\": \"RC-12A\",\n"
		   << "  \"clientPolicy\": \"enabled exactly at high shader quality\",\n"
		   << "  \"requestedMode\": \"" << DistortionModeName( options.distortion ) << "\",\n"
		   << "  \"resolvedMode\": \"" << DistortionModeName( options.resolvedDistortion ) << "\",\n"
		   << "  \"mapHash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' )
		   << diagnostics.mapHash << "\",\n"
		   << "  \"offPreTonemapHash\": \"" << std::setw( 16 ) << diagnostics.offPreTonemapHash << "\",\n"
		   << "  \"authoredPreTonemapHash\": \"" << std::setw( 16 ) << diagnostics.authoredPreTonemapHash
		   << "\",\n"
		   << "  \"offFinalHash\": \"" << std::setw( 16 ) << diagnostics.offFinalHash << "\",\n"
		   << "  \"authoredFinalHash\": \"" << std::setw( 16 ) << diagnostics.authoredFinalHash << std::dec
		   << "\",\n"
		   << "  \"dimensions\": [" << diagnostics.mapWidth << ", " << diagnostics.mapHeight << "],\n"
		   << "  \"format\": " << diagnostics.mapFormat << ",\n"
		   << "  \"submittedBatches\": " << diagnostics.submittedBatches << ",\n"
		   << "  \"submittedIndices\": " << diagnostics.submittedIndices << ",\n"
		   << "  \"foregroundApplications\": " << diagnostics.foregroundApplications << ",\n"
		   << "  \"backgroundApplications\": " << diagnostics.backgroundApplications << ",\n"
		   << "  \"neutralPixels\": " << diagnostics.neutralPixels << ",\n"
		   << "  \"nonNeutralPixels\": " << diagnostics.nonNeutralPixels << ",\n"
		   << "  \"saturatedPixels\": " << diagnostics.saturatedPixels << ",\n"
		   << "  \"redRange\": [" << static_cast<unsigned>( diagnostics.minimumRed ) << ", "
		   << static_cast<unsigned>( diagnostics.maximumRed ) << "],\n"
		   << "  \"greenRange\": [" << static_cast<unsigned>( diagnostics.minimumGreen ) << ", "
		   << static_cast<unsigned>( diagnostics.maximumGreen ) << "],\n"
		   << "  \"affectedBounds\": [" << diagnostics.affectedMinX << ", " << diagnostics.affectedMinY << ", "
		   << diagnostics.affectedMaxX << ", " << diagnostics.affectedMaxY << "],\n"
		   << "  \"mapCreated\": " << ( diagnostics.mapCreated ? "true" : "false" ) << ",\n"
		   << "  \"sceneColorCopySucceeded\": "
		   << ( diagnostics.copySucceeded ? "true" : "false" ) << ",\n"
		   << "  \"compositorSucceeded\": " << ( diagnostics.compositeSucceeded ? "true" : "false" ) << ",\n"
		   << "  \"validationPassed\": " << ( diagnostics.valid ? "true" : "false" ) << ",\n"
		   << "  \"generatedCmfHashPath\": \"" << generatedCmfHashPath << "\",\n"
		   << "  \"generatedCmfHash\": \"" << generatedCmfHash << "\",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Distortion contract report: " << outputPath << "\n";
	return output.good();
}

bool CapturePresentedProduct( void* probe, NSWindow* window, const Options& options, RenderProduct product )
{
	const std::string basePath = CaptureBasePath( options );
	const std::string suffix = product == RenderProduct::Window ? "" : "_" + RenderProductName( product );
	if( product != RenderProduct::Window )
	{
		return CaptureProbeProductPng( probe, basePath + suffix + ".png" );
	}
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	return CaptureWindowPng( window, basePath + suffix + ".png" );
}

}

@interface EveSceneProbeWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation EveSceneProbeWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
	(void)sender;
	g_shouldQuit = true;
	return YES;
}
@end

int main( int argc, char** argv )
{
	@autoreleasepool
	{
		Options options;
		if( !ParseArgs( argc, argv, options ) )
		{
			PrintUsage( argv[0] );
			return 2;
		}

		if( options.inspectionReportPath.empty() && options.qualityRung >= QualityRung::Model &&
			!FileExists( options.inputPath ) )
		{
			std::cerr << "CMF asset file is not present: " << options.inputPath << "\n";
			return 1;
		}

		const std::string executableDirectory = ExecutableDirectory();
		if( !TrinityStandaloneProbeStartup(
				argc, const_cast<const char* const*>( argv ), executableDirectory.c_str() ) )
		{
			std::cerr << "Trinity standalone probe startup failed\n";
			return 1;
		}

		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		NSView* view = nil;
		NSWindow* window = CreateWindow( options, &view );
		EveSceneProbeWindowDelegate* delegate = [[EveSceneProbeWindowDelegate alloc] init];
		[window setDelegate:delegate];
		if( options.backgroundCapture )
		{
			[window orderBack:nil];
		}
		else
		{
			[window makeKeyAndOrderFront:nil];
			[NSApp activateIgnoringOtherApps:YES];
		}

		const CGFloat backingScale = std::max( static_cast<CGFloat>( 1.0 ), [window backingScaleFactor] );
		const NSSize viewSize = view.bounds.size;
		const uint32_t renderWidth = GetBackingDimension( viewSize.width, backingScale );
		const uint32_t renderHeight = GetBackingDimension( viewSize.height, backingScale );
		if( [view.layer isKindOfClass:CAMetalLayer.class] )
		{
			CAMetalLayer* metalLayer = (CAMetalLayer*)view.layer;
			metalLayer.contentsScale = backingScale;
			metalLayer.drawableSize = CGSizeMake( renderWidth, renderHeight );
		}

		void* probe = TrinityStandaloneProbeCreateDevice(
			(__bridge void*)view, renderWidth, renderHeight, static_cast<int>( options.shaderTier ) );
		if( !probe )
		{
			std::cerr << "TrinityStandaloneProbeCreateDevice failed\n";
			[window close];
			return 1;
		}

		if( !options.inspectionReportPath.empty() )
		{
			bool inspectionSucceeded = false;
			@autoreleasepool
			{
				inspectionSucceeded = EnsureParentDirectory( options.inspectionReportPath ) &&
					TrinityStandaloneProbeInspectClientAssets( probe, options.inspectionReportPath.c_str() );
			}
			if( !inspectionSucceeded )
			{
				std::cerr << "TrinityStandaloneProbeInspectClientAssets failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			std::cout << "Client asset report: " << options.inspectionReportPath << "\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 0;
		}

		const int qualityRung = QualityRungApiValue( options.qualityRung );
		if( options.qualityRung != QualityRung::Shell &&
			!TrinityStandaloneProbeCreateEveScene( probe,
												   qualityRung,
												   options.inputPath.c_str(),
												   static_cast<int>( options.materialView ),
												   static_cast<int>( options.materialMode ),
												   static_cast<int>( options.areaView ),
												   SceneResourcePath( options.sceneFixture ),
												   SceneFixtureApiValue( options.sceneFixture ),
												   static_cast<int>( options.lightingView ),
												   static_cast<int>( options.shSource ),
												   static_cast<int>( options.localLights ),
												   LocalShadowsApiValue( options.resolvedLocalShadows ),
												   ReflectionSourceApiValue( options.resolvedReflectionSource ),
												   static_cast<int>( options.reflectionCorrection ),
												   static_cast<int>( options.normalMapMode ),
												   options.resolvedDistortion == DistortionMode::Authored ? 1 : 0,
												   static_cast<int>( options.cameraView ),
												   static_cast<int>( options.composition ),
												   static_cast<int>( options.planetLayers ),
												   options.cloudYear,
												   options.cloudMonth,
												   options.cloudDay,
												   static_cast<int>( options.resolvedSunEffects ),
												   static_cast<int>( options.resolvedAttachments ),
												   static_cast<int>( options.attachmentView ),
												   static_cast<int>( options.resolvedDecals ),
												   static_cast<int>( options.decalView ),
												   options.killCount,
												   options.modelYawDegrees,
												   ShadowsApiValue( options.resolvedShadows ),
												   AmbientOcclusionApiValue( options.resolvedAmbientOcclusion ),
												   static_cast<int>( options.aoMethod ) ) )
		{
			std::cerr << "TrinityStandaloneProbeCreateEveScene failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		const bool collectExposureDiagnostics = options.validateExposureTone || options.validatePostFinish ||
			options.validateDistortion ||
			options.exposureSequence != ExposureSequence::None;
		const bool captureToneSnapshot = options.qualityRung >= QualityRung::HdrExposure &&
			!options.capturePrefix.empty() &&
			( options.validateExposureTone || options.renderProduct == RenderProduct::PostTonemap ||
			  options.renderProduct == RenderProduct::All );
		if( options.qualityRung >= QualityRung::HdrExposure &&
			!TrinityStandaloneProbeConfigurePostProcess(
				probe,
				options.resolvedDynamicExposure == DynamicExposure::Client ? 1 : 0,
				options.resolvedBloom == PostFinishMode::Client ? 1 : 0,
				options.resolvedFilmGrain == PostFinishMode::Client ? 1 : 0,
				collectExposureDiagnostics || captureToneSnapshot ) )
		{
			std::cerr << "TrinityStandaloneProbeConfigurePostProcess failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}

		int renderedFrames = 0;
		bool compositionValidationAttempted = false;
		bool compositionValidationSucceeded = true;
		std::string compositionValidationStats;
		auto validateCompositionAtTime = [&]( int64_t validationTime ) {
			const bool passed =
				TrinityStandaloneProbeRenderFrame( probe,
												   qualityRung,
												   validationTime,
												   validationTime,
												   kCaptureAmbientOcclusion | kCaptureFreezeScene ) &&
				TrinityStandaloneProbeRenderFrame(
					probe, qualityRung, validationTime, validationTime, kCaptureHdrComposite | kCaptureFreezeScene ) &&
				TrinityStandaloneProbeValidateComposition( probe );
			const char* summary = TrinityStandaloneProbeGetCompositionValidationSummary( probe );
			compositionValidationStats = summary ? summary : "profile=RC-09 validation=fail summary=unavailable";
			return passed && summary;
		};
		const uint32_t compositionWarmupFrames = std::max( options.timingWarmup, 2u );
		std::vector<double> framePacingSamples;
		std::vector<TrinityStandalonePostProcessDiagnostics> exposureSamples;
		while( !g_shouldQuit && ( options.maxFrames < 0 || renderedFrames < options.maxFrames ) )
		{
			@autoreleasepool
			{
				ProcessEvents( window );
				if( g_shouldQuit )
				{
					break;
				}

				const int64_t realTime = static_cast<int64_t>( renderedFrames ) * kFrameTime;
				const int64_t simTime = realTime;
				if( options.exposureSequence != ExposureSequence::None )
				{
					const bool secondPhase = static_cast<uint32_t>( renderedFrames ) >= options.exposureHold;
					const bool sunFacing = options.exposureSequence == ExposureSequence::DarkToBright ?
						secondPhase : !secondPhase;
					if( !TrinityStandaloneProbeSetExposureCameraPhase( probe, true, sunFacing ) )
					{
						std::cerr << "Failed to set the RC-10 exposure camera phase\n";
						TrinityStandaloneProbeDestroyDevice( probe );
						[window close];
						return 1;
					}
				}
				const int captureProducts = options.maxFrames > 0 && renderedFrames + 1 == options.maxFrames ?
					RenderProductApiValue( options.renderProduct == RenderProduct::All ? RenderProduct::Color :
																						 options.renderProduct ) :
					0;
				const auto frameStart = std::chrono::steady_clock::now();
				const bool rendered =
					TrinityStandaloneProbeRenderFrame( probe, qualityRung, realTime, simTime, captureProducts );
				const auto frameEnd = std::chrono::steady_clock::now();
				if( !rendered )
				{
					std::cerr << "TrinityStandaloneProbeRenderFrame failed\n";
					TrinityStandaloneProbeDestroyDevice( probe );
					[window close];
					return 1;
				}
				if( collectExposureDiagnostics )
				{
					TrinityStandalonePostProcessDiagnostics diagnostics;
					if( !TrinityStandaloneProbeGetPostProcessDiagnostics( probe, &diagnostics ) )
					{
						std::cerr << "Failed to read RC-10 exposure diagnostics\n";
						TrinityStandaloneProbeDestroyDevice( probe );
						[window close];
						return 1;
					}
					exposureSamples.push_back( diagnostics );
				}
				if( options.framePacingCheck && static_cast<uint32_t>( renderedFrames ) >= options.timingWarmup &&
					captureProducts == 0 )
				{
					framePacingSamples.push_back(
						std::chrono::duration<double, std::milli>( frameEnd - frameStart ).count() );
				}
				++renderedFrames;
				if( options.validateComposition && !compositionValidationAttempted &&
					static_cast<uint32_t>( renderedFrames ) == compositionWarmupFrames )
				{
					compositionValidationAttempted = true;
					compositionValidationSucceeded = validateCompositionAtTime( realTime );
					if( !compositionValidationSucceeded )
					{
						break;
					}
				}
			}
		}

		if( options.validateComposition && !compositionValidationAttempted )
		{
			const int64_t validationTime = static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
			compositionValidationAttempted = true;
			compositionValidationSucceeded = validateCompositionAtTime( validationTime );
		}

		std::string framePacingStats;
		const bool framePacingSucceeded =
			!options.framePacingCheck || EvaluateFramePacing( framePacingSamples, framePacingStats );
		std::string exposureValidationStats = "not-requested";
		const bool exposureValidationSucceeded = !collectExposureDiagnostics ||
			EvaluateExposureSamples( options, exposureSamples, exposureValidationStats );
		TrinityStandaloneToneValidation toneValidation;
		TrinityStandaloneToneValidation offToneValidation;
		bool toneValidationSucceeded = true;
		std::string toneComparisonStats = "not-requested";
		const bool runMatchedToneComparison = options.validateExposureTone &&
			options.exposureSequence == ExposureSequence::None &&
			static_cast<uint32_t>( renderedFrames ) == options.exposureHold;
		if( captureToneSnapshot )
		{
			if( options.qualityRung == QualityRung::HdrFinish &&
				!TrinityStandaloneProbeConfigurePostProcess(
					probe, options.resolvedDynamicExposure == DynamicExposure::Client ? 1 : 0, 0, 0, true ) )
			{
				toneValidationSucceeded = false;
			}
			const int64_t captureTime = static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
			toneValidationSucceeded =
				toneValidationSucceeded &&
				TrinityStandaloneProbeRenderFrame(
					probe, qualityRung, captureTime, captureTime, kCaptureToneContract | kCaptureFreezeScene ) &&
				TrinityStandaloneProbeGetToneValidation( probe, &toneValidation ) && toneValidation.valid;
			if( toneValidationSucceeded && options.exposureSequence == ExposureSequence::None )
			{
				const uint64_t pixelCount = static_cast<uint64_t>( toneValidation.width ) * toneValidation.height;
				toneValidationSucceeded = toneValidation.fullyWhitePixels <= pixelCount / 1000 &&
					toneValidation.anySaturatedChannelPixels <= pixelCount * 3 / 100 &&
					toneValidation.fullyBlackPixels <= pixelCount / 1000;
			}
			toneValidation.valid = toneValidationSucceeded;
			if( toneValidationSucceeded && runMatchedToneComparison )
			{
				toneValidationSucceeded =
					TrinityStandaloneProbeConfigurePostProcess( probe, 0, 0, 0, true ) &&
					TrinityStandaloneProbeRenderFrame(
						probe, qualityRung, captureTime, captureTime, kCaptureToneContract | kCaptureFreezeScene ) &&
					TrinityStandaloneProbeGetToneValidation( probe, &offToneValidation );
				const bool restored = TrinityStandaloneProbeConfigurePostProcess( probe, 1, 0, 0, true );
				if( toneValidationSucceeded )
				{
					toneValidationSucceeded = CompareToneValidations(
						offToneValidation, toneValidation, toneComparisonStats );
				}
				toneValidationSucceeded = restored && toneValidationSucceeded;
			}
			if( options.qualityRung == QualityRung::HdrFinish )
			{
				toneValidationSucceeded = TrinityStandaloneProbeConfigurePostProcess(
											  probe,
											  options.resolvedDynamicExposure == DynamicExposure::Client ? 1 : 0,
											  options.resolvedBloom == PostFinishMode::Client ? 1 : 0,
											  options.resolvedFilmGrain == PostFinishMode::Client ? 1 : 0,
											  true ) &&
					toneValidationSucceeded;
			}
		}
		TrinityStandalonePostFinishValidation postFinishValidation;
		bool postFinishValidationSucceeded = true;
		if( options.validatePostFinish )
		{
			const int64_t captureTime = static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
			postFinishValidationSucceeded =
				TrinityStandaloneProbeRenderFrame(
					probe, qualityRung, captureTime, captureTime, kCapturePostFinishContract | kCaptureFreezeScene ) &&
				TrinityStandaloneProbeGetPostFinishValidation( probe, &postFinishValidation ) &&
				postFinishValidation.valid;
		}
		TrinityStandaloneDistortionDiagnostics distortionDiagnostics;
		bool distortionValidationSucceeded = true;
		if( options.validateDistortion )
		{
			distortionValidationSucceeded = TrinityStandaloneProbeValidateDistortion( probe ) &&
				TrinityStandaloneProbeGetDistortionDiagnostics( probe, &distortionDiagnostics ) &&
				distortionDiagnostics.valid;
		}
		bool exposureReportsSucceeded = true;
		if( collectExposureDiagnostics )
		{
			exposureReportsSucceeded = WriteExposureCsv(
				CaptureBasePath( options ) + "_exposure.csv", exposureSamples );
		}
		if( captureToneSnapshot )
		{
			exposureReportsSucceeded = WriteToneContractJson(
				options,
				toneValidation,
				exposureValidationStats,
				options.resolvedDynamicExposure,
				"_tone-contract.json" ) &&
				exposureReportsSucceeded;
			if( runMatchedToneComparison )
			{
				exposureReportsSucceeded = WriteToneContractJson(
					options,
					offToneValidation,
					"paired-control",
					DynamicExposure::Off,
					"_tone-contract-off.json" ) &&
					exposureReportsSucceeded;
			}
		}
		if( options.validatePostFinish )
		{
			exposureReportsSucceeded =
				WritePostFinishContractJson( options, postFinishValidation ) && exposureReportsSucceeded;
		}
		if( options.validateDistortion )
		{
			exposureReportsSucceeded =
				WriteDistortionContractJson( options, distortionDiagnostics ) && exposureReportsSucceeded;
		}
		bool captureSucceeded = true;
		@autoreleasepool
		{
			std::vector<std::pair<std::string, std::string>> productStats;
			if( options.framePacingCheck )
			{
				productStats.emplace_back( "framePacing", framePacingStats );
			}
			if( options.validateComposition )
			{
				productStats.emplace_back( "compositionValidation", compositionValidationStats );
			}
			if( collectExposureDiagnostics )
			{
				productStats.emplace_back( "exposureValidation", exposureValidationStats );
			}
			if( captureToneSnapshot )
			{
				std::ostringstream stats;
				stats << "preHash=" << std::hex << std::setw( 16 ) << std::setfill( '0' )
					  << toneValidation.preTonemapHash << " postHash=" << std::setw( 16 )
					  << toneValidation.postTonemapHash << std::dec << " fullyBlack="
					  << toneValidation.fullyBlackPixels << " fullyWhite=" << toneValidation.fullyWhitePixels
					  << " anySaturated=" << toneValidation.anySaturatedChannelPixels
					  << " errorMean=" << toneValidation.meanAbsoluteError
					  << " errorP999=" << toneValidation.p999AbsoluteError
					  << " errorMax=" << toneValidation.maximumAbsoluteError
					  << " validation=" << ( toneValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "toneValidation", stats.str() );
				if( runMatchedToneComparison )
				{
					productStats.emplace_back( "toneComparison", toneComparisonStats );
				}
			}
			if( options.validatePostFinish )
			{
				std::ostringstream stats;
				stats << "bloomHash=" << std::hex << std::setw( 16 ) << std::setfill( '0' )
					  << postFinishValidation.bloomHash << " postHash=" << std::setw( 16 )
					  << postFinishValidation.postTonemapHash << " finalHash=" << std::setw( 16 )
					  << postFinishValidation.finalHash << std::dec << " bloom=" << postFinishValidation.bloomWidth
					  << "x" << postFinishValidation.bloomHeight
					  << " bloomNonzero=" << postFinishValidation.bloomNonzeroPixels
					  << " grainChanged=" << postFinishValidation.grainChangedPixels
					  << " grainP99=" << postFinishValidation.grainP99AbsoluteError
					  << " validation=" << ( postFinishValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "postFinishValidation", stats.str() );
			}
			if( options.validateDistortion )
			{
				std::ostringstream stats;
				stats << "mapHash=" << std::hex << std::setw( 16 ) << std::setfill( '0' )
					  << distortionDiagnostics.mapHash << " offPre=" << std::setw( 16 )
					  << distortionDiagnostics.offPreTonemapHash << " authoredPre=" << std::setw( 16 )
					  << distortionDiagnostics.authoredPreTonemapHash << " offFinal=" << std::setw( 16 )
					  << distortionDiagnostics.offFinalHash << " authoredFinal=" << std::setw( 16 )
					  << distortionDiagnostics.authoredFinalHash << std::dec << " map="
					  << distortionDiagnostics.mapWidth << "x" << distortionDiagnostics.mapHeight
					  << " nonNeutral=" << distortionDiagnostics.nonNeutralPixels
					  << " affectedBounds=" << distortionDiagnostics.affectedMinX << ","
					  << distortionDiagnostics.affectedMinY << "-" << distortionDiagnostics.affectedMaxX << ","
					  << distortionDiagnostics.affectedMaxY
					  << " validation=" << ( distortionValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "distortionValidation", stats.str() );
			}
			if( !options.capturePrefix.empty() )
			{
				ProcessEvents( window );
				if( options.renderProduct == RenderProduct::All )
				{
					captureSucceeded = CapturePresentedProduct( probe, window, options, RenderProduct::Color );
					productStats.emplace_back( "color", RenderProductStats( options, RenderProduct::Color ) );
					std::vector<RenderProduct> diagnosticProducts = { RenderProduct::Depth,
																	  RenderProduct::Normal,
																	  RenderProduct::Reflection };
					if( options.qualityRung >= QualityRung::HdrPost )
					{
						diagnosticProducts.push_back( RenderProduct::HdrComposite );
					}
					if( options.qualityRung >= QualityRung::HdrExposure )
					{
						diagnosticProducts.push_back( RenderProduct::PostTonemap );
					}
					if( options.qualityRung == QualityRung::HdrFinish )
					{
						diagnosticProducts.push_back( RenderProduct::Bloom );
						diagnosticProducts.push_back( RenderProduct::FinalPostprocess );
						if( options.resolvedDistortion == DistortionMode::Authored )
						{
							diagnosticProducts.push_back( RenderProduct::Distortion );
						}
					}
					if( options.resolvedShadows != Shadows::Off )
					{
						diagnosticProducts.push_back( RenderProduct::Shadow );
						diagnosticProducts.push_back( RenderProduct::ShadowAtlas );
					}
					if( options.resolvedLocalShadows != LocalShadows::Off )
					{
						diagnosticProducts.push_back( RenderProduct::LocalShadowAtlas );
					}
					if( options.resolvedAmbientOcclusion != AmbientOcclusion::Off )
					{
						diagnosticProducts.push_back( RenderProduct::AmbientOcclusion );
						if( options.aoMethod == AoMethod::Cortao )
						{
							diagnosticProducts.push_back( RenderProduct::BentNormal );
						}
					}
					for( RenderProduct product : diagnosticProducts )
					{
						const int64_t captureTime =
							static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
						const int frozenProduct = RenderProductApiValue( product ) | kCaptureFreezeScene;
						bool productRendered = captureSucceeded &&
							TrinityStandaloneProbeRenderFrame(
												   probe, qualityRung, captureTime, captureTime, frozenProduct );
						if( productRendered && product != RenderProduct::HdrComposite )
						{
							productRendered = TrinityStandaloneProbeRenderFrame(
								probe, qualityRung, captureTime, captureTime, frozenProduct );
						}
						if( !productRendered )
						{
							captureSucceeded = false;
							break;
						}
						ProcessEvents( window );
						captureSucceeded = CapturePresentedProduct( probe, window, options, product );
						productStats.emplace_back( RenderProductName( product ),
												   RenderProductStats( options, product ) );
					}
				}
				else
				{
					const bool capturedHdrOnFinalFrame = options.renderProduct == RenderProduct::HdrComposite &&
						options.maxFrames > 0 && renderedFrames == options.maxFrames;
					if( options.renderProduct != RenderProduct::Window &&
						options.renderProduct != RenderProduct::Color && !capturedHdrOnFinalFrame )
					{
						const int64_t captureTime =
							static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
						captureSucceeded = TrinityStandaloneProbeRenderFrame(
							probe,
							qualityRung,
							captureTime,
							captureTime,
							RenderProductApiValue( options.renderProduct ) | kCaptureFreezeScene );
						ProcessEvents( window );
					}
					captureSucceeded =
						captureSucceeded && CapturePresentedProduct( probe, window, options, options.renderProduct );
					if( options.renderProduct != RenderProduct::Window )
					{
						productStats.emplace_back( RenderProductName( options.renderProduct ),
												   RenderProductStats( options, options.renderProduct ) );
					}
				}
				if( captureSucceeded &&
					( options.renderProduct == RenderProduct::HdrComposite ||
					  ( options.renderProduct == RenderProduct::All && options.qualityRung >= QualityRung::HdrPost ) ) )
				{
					captureSucceeded = AppendHdrCompositeStats( probe, productStats );
				}
				if( options.resolvedShadows != Shadows::Off )
				{
					uint32_t casterTests = 0;
					uint32_t acceptedCascades = 0;
					uint32_t committedBatches = 0;
					if( !TrinityStandaloneProbeGetShadowDiagnostics(
							probe, &casterTests, &acceptedCascades, &committedBatches ) )
					{
						captureSucceeded = false;
					}
					else
					{
						std::ostringstream stats;
						stats << "casterTests=" << casterTests << " acceptedCascades=" << acceptedCascades
							  << " committedBatches=" << committedBatches
							  << " expectedBatches=" << acceptedCascades * 2;
						productStats.emplace_back( "shadowRuntime", stats.str() );
					}
				}
				if( options.resolvedLocalShadows != LocalShadows::Off )
				{
					uint32_t eligibleLights = 0;
					uint32_t selectedLights = 0;
					uint32_t droppedLights = 0;
					uint32_t packedEntries = 0;
					uint32_t hazeEligible = 0;
					uint32_t bannerEligible = 0;
					uint32_t validationEligible = 0;
					uint32_t pointLights = 0;
					uint32_t spotLights = 0;
					uint32_t facePasses = 0;
					uint32_t casterTests = 0;
					uint32_t acceptedCasterPasses = 0;
					uint32_t committedBatches = 0;
					uint32_t atlasFormat = 0;
					uint32_t atlasWidth = 0;
					uint32_t atlasHeight = 0;
					bool allocationSucceeded = false;
					bool atlasValid = false;
					if( !TrinityStandaloneProbeGetLocalShadowDiagnostics( probe,
																		  &eligibleLights,
																		  &selectedLights,
																		  &droppedLights,
																		  &packedEntries,
																		  &hazeEligible,
																		  &bannerEligible,
																		  &validationEligible,
																		  &pointLights,
																		  &spotLights,
																		  &facePasses,
																		  &casterTests,
																		  &acceptedCasterPasses,
																		  &committedBatches,
																		  &atlasFormat,
																		  &atlasWidth,
																		  &atlasHeight,
																		  &allocationSucceeded,
																		  &atlasValid ) )
					{
						captureSucceeded = false;
					}
					else
					{
						std::ostringstream stats;
						stats << "eligible=" << eligibleLights << " selected=" << selectedLights
							  << " dropped=" << droppedLights << " packed=" << packedEntries
							  << " hazeEligible=" << hazeEligible << " bannerEligible=" << bannerEligible
							  << " validationEligible=" << validationEligible << " point=" << pointLights
							  << " spot=" << spotLights << " faces=" << facePasses << " casterTests=" << casterTests
							  << " accepted=" << acceptedCasterPasses << " batches=" << committedBatches
							  << " atlasFormat=" << atlasFormat << " atlas=" << atlasWidth << "x" << atlasHeight
							  << " allocation=" << ( allocationSucceeded ? "success" : "failed" )
							  << " atlasValid=" << ( atlasValid ? "true" : "false" );
						productStats.emplace_back( "localShadowRuntime", stats.str() );
					}
				}
				if( options.qualityRung != QualityRung::Shell )
				{
					int reflectionSource = -1;
					uint32_t reflectionFormat = 0;
					uint32_t reflectionWidth = 0;
					uint32_t reflectionHeight = 0;
					uint32_t reflectionMipCount = 0;
					bool reflectionHasData = false;
					bool reflectionFilterSucceeded = false;
					float reflectionIntensity = 0.0f;
					float shPrimaryIntensity = 0.0f;
					float shSecondaryIntensity = 0.0f;
					if( !TrinityStandaloneProbeGetReflectionDiagnostics( probe,
																		 &reflectionSource,
																		 &reflectionFormat,
																		 &reflectionWidth,
																		 &reflectionHeight,
																		 &reflectionMipCount,
																		 &reflectionHasData,
																		 &reflectionFilterSucceeded,
																		 &reflectionIntensity,
																		 &shPrimaryIntensity,
																		 &shSecondaryIntensity ) )
					{
						captureSucceeded = false;
					}
					else
					{
						std::ostringstream stats;
						stats << "source=" << ReflectionSourceName( options.resolvedReflectionSource )
							  << " sourceApi=" << reflectionSource << " format=" << reflectionFormat
							  << " dimensions=" << reflectionWidth << "x" << reflectionHeight
							  << " mipCount=" << reflectionMipCount
							  << " hasData=" << ( reflectionHasData ? "true" : "false" )
							  << " filterSucceeded=" << ( reflectionFilterSucceeded ? "true" : "false" )
							  << " reflectionIntensity=" << reflectionIntensity
							  << " shPrimaryIntensity=" << shPrimaryIntensity
							  << " shSecondaryIntensity=" << shSecondaryIntensity;
						productStats.emplace_back( "reflectionRuntime", stats.str() );
					}
				}
				captureSucceeded = captureSucceeded &&
					WriteCaptureMetadata( options, renderWidth, renderHeight, renderedFrames, productStats );
			}
		}
		captureSucceeded = captureSucceeded && framePacingSucceeded && compositionValidationSucceeded &&
			exposureValidationSucceeded && toneValidationSucceeded && postFinishValidationSucceeded &&
			distortionValidationSucceeded && exposureReportsSucceeded;
		if( !captureSucceeded )
		{
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}

		TrinityStandaloneProbeDestroyDevice( probe );
		[window close];
	}

	return 0;
}
