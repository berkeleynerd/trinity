// Copyright (c) 2026 CCP Games

#import <Cocoa/Cocoa.h>
#import <ImageIO/ImageIO.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "TrinityStandaloneProbeApi.h"

extern "C" bool TrinityStandaloneProbeStartup( int argc, const char* const* argv, const char* executableDirectory );
extern "C" void* TrinityStandaloneProbeCreateDevice( void* windowHandle,
													 uint32_t renderWidth,
													 uint32_t renderHeight,
													 int shaderTier );
extern "C" void TrinityStandaloneProbeDestroyDevice( void* opaqueProbe );
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
extern "C" bool TrinityStandaloneProbeAuthorizeFroxelLab( void* opaqueProbe, const char* ledgerPath );
#endif
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
extern "C" bool TrinityStandaloneProbeConfigureSolarAudit( void* opaqueProbe, const char* reportPath );
extern "C" bool TrinityStandaloneProbeWriteSolarAudit( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeConfigureSolarBody( void* opaqueProbe,
														  int layerMode,
														  const char* reportPath,
														  const char* resourceManifestPath,
														  const char* geometryManifestPath,
														  const char* generatedGeometryHashPath );
extern "C" bool TrinityStandaloneProbeWriteSolarBodyReport( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeConfigureSolarHigh( void* opaqueProbe,
												 int layerMode,
												 const char* reportPath,
												 const char* resourceManifestPath,
												 const char* geometryManifestPath,
												 const char* generatedGeometryManifestPath,
												 uint32_t particleSeed,
												 bool naturalFirstEmissionPrewarm );
extern "C" bool TrinityStandaloneProbePrewarmSolarParticles( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeWriteSolarHighReport( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeWarmupSolarHigh( void* opaqueProbe, uint32_t ticks );
extern "C" bool TrinityStandaloneProbeConfigureSolarIllumination( void* opaqueProbe,
	int illuminationMode,
	int illuminationView,
	const char* reportPath );
extern "C" bool TrinityStandaloneProbeSetSolarIlluminationCaptureRequested(
	void* opaqueProbe,
	bool requested );
extern "C" bool TrinityStandaloneProbeWriteSolarIlluminationReport( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeConfigureSolarOptics( void* opaqueProbe,
														 int environmentMode,
														 int environmentDistance,
														 bool sceneDistanceFogAuthored,
														 bool desaturateAuthored,
														 int occlusionView,
														 const char* reportPath );
extern "C" bool TrinityStandaloneProbeWriteSolarOpticsReport( void* opaqueProbe );
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
													  int engines,
													  int engineView,
													  float engineThrottle,
													  float modelYawDegrees,
													  int taaMode,
													  int taaDebug,
													  int motionMode,
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
	VolumeSlices,
	FroxelFog,
	MieEnvironment,
	Velocity,
	TaaInput,
	TaaOutput,
	TaaCooldown,
	All,
};

enum class Shadows
{
	Auto,
	Off,
	Low,
	High,
	Raytraced,
};

enum class SolarIllumination
{
	Frozen,
	Authored,
	Off,
};

enum class SolarIlluminationView
{
	Unspecified = -1,
	NearSunHull = 0,
	GateHull,
	PlanetDay,
	PlanetLimb,
	PlanetEclipse,
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
	SunFlares,
	LensFlare,
};

enum class SolarEnvironment
{
	Off,
	Fog,
	Finish,
	All,
};

enum class SolarEnvironmentDistance
{
	Canonical,
	InsideBoundary,
	OutsideBoundary,
	ExitBoundary,
};

enum class AuthoredToggle
{
	Authored,
	Off,
};

enum class SolarOcclusionView
{
	Unobscured,
	HullPartial,
	HullTotal,
	PlanetLimb,
	PlanetEclipse,
	HullSweep,
	PlanetSweep,
};

enum class SunBodyLayers
{
	Off,
	Surface,
	OuterBeams,
	EdgeClouds,
	Corona,
	All,
};

enum class SunHighLayers
{
	Off,
	Whisps,
	HugeDefinition,
	Uber1,
	Uber2,
	RingsTop,
	RingsBottom,
	Rings,
	Pillar1,
	Pillar2,
	ParticleScaler,
	Particles,
	All,
};

enum class SolarParticlePrewarm
{
	None,
	NaturalFirstEmission,
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
constexpr int kCaptureVolumeSlices = 1 << 17;
constexpr int kCaptureFroxelFog = 1 << 18;
constexpr int kCaptureMieEnvironment = 1 << 19;
constexpr int kCaptureVelocity = 1 << 20;
constexpr int kCaptureTaaInput = 1 << 21;
constexpr int kCaptureTaaOutput = 1 << 22;
constexpr int kCaptureTaaCooldown = 1 << 23;
constexpr int kCaptureTemporalSample = 1 << 24;

enum class DynamicExposure
{
	Auto,
	Off,
	Client,
};

enum class TaaMode
{
	Auto,
	Off,
	Low,
	Medium,
	High,
};

enum class TaaDebug
{
	Off,
	MotionVectors,
	EarlyOut,
};

enum class MotionMode
{
	Static,
	Camera,
	Object,
	Combined,
};

enum class BallparkMode
{
	Off,
	Static,
	Goto,
	Orbit,
	Warp,
	Approach,
};

enum class WarpTarget
{
	Planet,
	EveGate,
};

enum class OrbitSolver
{
	Legacy,
	New,
};

enum class BallparkFrame
{
	Ego,
	Observer,
	Chase,
};

enum class CelestialBallparkMode
{
	Off,
	Natural,
};

enum class TemporalTest
{
	Contract,
	Velocity,
	Edges,
	Silk,
	Trails,
	Integrated,
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

enum class EngineMode
{
	Auto,
	Off,
	Authored,
};

enum class EngineView
{
	All,
	Plumes,
	Glows,
	Trails,
	Lights,
};

enum class VolumetricMode
{
	Auto,
	Off,
	Silk,
	Froxel,
	All,
};

enum class VolumetricQuality
{
	Auto,
	Low,
	Medium,
	High,
	Ultra,
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
	std::string solarAuditReportPath;
	std::string solarBodyReportPath;
	std::string solarHighReportPath;
	std::string solarOpticsReportPath;
	std::string solarIlluminationReportPath;
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
	EngineMode engines = EngineMode::Auto;
	EngineMode resolvedEngines = EngineMode::Off;
	EngineView engineView = EngineView::All;
	float engineThrottle = 1.0f;
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
	SolarIllumination solarIllumination = SolarIllumination::Frozen;
	SolarIlluminationView solarIlluminationView = SolarIlluminationView::Unspecified;
	bool solarIlluminationExplicit = false;
	bool solarIlluminationViewExplicit = false;
	SolarEnvironment solarEnvironment = SolarEnvironment::Off;
	SolarEnvironmentDistance solarEnvironmentDistance = SolarEnvironmentDistance::Canonical;
	AuthoredToggle sceneDistanceFog = AuthoredToggle::Authored;
	AuthoredToggle solarDesaturate = AuthoredToggle::Authored;
	SolarOcclusionView solarOcclusionView = SolarOcclusionView::Unobscured;
	SunBodyLayers sunBodyLayers = SunBodyLayers::All;
	SunHighLayers sunHighLayers = SunHighLayers::Off;
	bool sunHighLayersExplicit = false;
	uint32_t solarParticleSeed = 3430261;
	bool solarParticleSeedExplicit = false;
	SolarParticlePrewarm solarParticlePrewarm = SolarParticlePrewarm::None;
	uint32_t solarHighWarmup = 0;
	bool journeyPlanetFinale = false;
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
	bool deterministicEvidence = false;
	bool validateComposition = false;
	bool validateExposureTone = false;
	bool validatePostFinish = false;
	bool validateDistortion = false;
	bool validateEngines = false;
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
	VolumetricMode volumetrics = VolumetricMode::Auto;
	VolumetricMode resolvedVolumetrics = VolumetricMode::Off;
	VolumetricQuality volumetricQuality = VolumetricQuality::Auto;
	VolumetricQuality resolvedVolumetricQuality = VolumetricQuality::High;
	uint32_t volumetricSeed = 0x12b;
	bool enableFroxels = false;
	bool froxelTemporal = true;
	bool validateVolumetrics = false;
	bool clientKernels = false;
	std::string froxelLabLedger;
	TaaMode taa = TaaMode::Auto;
	TaaMode resolvedTaa = TaaMode::Off;
	TaaDebug taaDebug = TaaDebug::Off;
	MotionMode motion = MotionMode::Camera;
	BallparkMode ballpark = BallparkMode::Off;
	WarpTarget warpTarget = WarpTarget::Planet;
	BallparkFrame ballparkFrame = BallparkFrame::Ego;
	bool validateBallpark = false;
	bool validateBallparkMotion = false;
	bool validateBallparkOrbit = false;
	bool validateBallparkWarp = false;
	bool validateBallparkApproach = false;
	int captureEvery = 0;
	int captureStartFrame = 0;
	std::vector<int> captureFrames;
	OrbitSolver orbitSolver = OrbitSolver::New;
	float orbitRange = 2500.0f;
	bool validateChaseCamera = false;
	std::string ballparkLogPath;
	CelestialBallparkMode celestialBallpark = CelestialBallparkMode::Off;
	bool validateCelestialBallpark = false;
	std::string celestialLogPath;
	uint64_t eveGateApproachFrame = 0;
	int eveGate = 0;
	int warpTunnel = 0;
	bool validateEveGate = false;
	int celestialAnchor = 0;
	float eveGateRatio = -1.0f;
	int eveGateTravel = 0;
	bool validateTemporal = false;
	TemporalTest temporalTest = TemporalTest::Contract;
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
	case RenderProduct::VolumeSlices:
		return "volume-slices";
	case RenderProduct::FroxelFog:
		return "froxel-fog";
	case RenderProduct::MieEnvironment:
		return "mie-environment";
	case RenderProduct::Velocity:
		return "velocity";
	case RenderProduct::TaaInput:
		return "taa-input";
	case RenderProduct::TaaOutput:
		return "taa-output";
	case RenderProduct::TaaCooldown:
		return "taa-cooldown";
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
									 RenderProduct::VolumeSlices,
									 RenderProduct::FroxelFog,
									 RenderProduct::MieEnvironment,
									 RenderProduct::Velocity,
									 RenderProduct::TaaInput,
									 RenderProduct::TaaOutput,
									 RenderProduct::TaaCooldown,
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

std::string TaaModeName( TaaMode value )
{
	static const char* names[] = { "auto", "off", "low", "medium", "high" };
	return names[static_cast<int>( value )];
}

bool ParseTaaMode( const std::string& value, TaaMode& result )
{
	const std::string normalized = ToLower( value );
	for( TaaMode candidate : { TaaMode::Auto, TaaMode::Off, TaaMode::Low, TaaMode::Medium, TaaMode::High } )
	{
		if( normalized == TaaModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string TaaDebugName( TaaDebug value )
{
	static const char* names[] = { "off", "motion-vectors", "early-out" };
	return names[static_cast<int>( value )];
}

bool ParseTaaDebug( const std::string& value, TaaDebug& result )
{
	const std::string normalized = ToLower( value );
	for( TaaDebug candidate : { TaaDebug::Off, TaaDebug::MotionVectors, TaaDebug::EarlyOut } )
	{
		if( normalized == TaaDebugName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string MotionModeName( MotionMode value )
{
	static const char* names[] = { "static", "camera", "object", "combined" };
	return names[static_cast<int>( value )];
}

std::string BallparkModeName( BallparkMode value )
{
	static const char* names[] = { "off", "static", "goto", "orbit", "warp", "approach" };
	return names[static_cast<int>( value )];
}

std::string BallparkFrameName( BallparkFrame value )
{
	static const char* names[] = { "ego", "observer", "chase" };
	return names[static_cast<int>( value )];
}

bool ParseBallparkMode( const std::string& value, BallparkMode& result )
{
	const std::string normalized = ToLower( value );
	if( normalized == "off" )
	{
		result = BallparkMode::Off;
		return true;
	}
	if( normalized == "static" )
	{
		result = BallparkMode::Static;
		return true;
	}
	if( normalized == "goto" )
	{
		result = BallparkMode::Goto;
		return true;
	}
	if( normalized == "orbit" )
	{
		result = BallparkMode::Orbit;
		return true;
	}
	if( normalized == "warp" )
	{
		result = BallparkMode::Warp;
		return true;
	}
	if( normalized == "approach" )
	{
		result = BallparkMode::Approach;
		return true;
	}
	return false;
}

bool ParseOrbitSolver( const std::string& value, OrbitSolver& result )
{
	const std::string normalized = ToLower( value );
	if( normalized == "legacy" )
	{
		result = OrbitSolver::Legacy;
		return true;
	}
	if( normalized == "new" )
	{
		result = OrbitSolver::New;
		return true;
	}
	return false;
}

bool ParseBallparkFrame( const std::string& value, BallparkFrame& result )
{
	const std::string normalized = ToLower( value );
	if( normalized == "ego" )
	{
		result = BallparkFrame::Ego;
		return true;
	}
	if( normalized == "observer" )
	{
		result = BallparkFrame::Observer;
		return true;
	}
	if( normalized == "chase" )
	{
		result = BallparkFrame::Chase;
		return true;
	}
	return false;
}

bool ParseMotionMode( const std::string& value, MotionMode& result )
{
	const std::string normalized = ToLower( value );
	for( MotionMode candidate : { MotionMode::Static, MotionMode::Camera, MotionMode::Object, MotionMode::Combined } )
	{
		if( normalized == MotionModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string TemporalTestName( TemporalTest value )
{
	static const char* names[] = { "contract", "velocity", "edges", "silk", "trails", "integrated" };
	return names[static_cast<int>( value )];
}

bool ParseTemporalTest( const std::string& value, TemporalTest& result )
{
	const std::string normalized = ToLower( value );
	for( TemporalTest candidate : { TemporalTest::Contract,
									TemporalTest::Velocity,
									TemporalTest::Edges,
									TemporalTest::Silk,
									TemporalTest::Trails,
									TemporalTest::Integrated } )
	{
		if( normalized == TemporalTestName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

TaaMode ResolveClientTaa( TaaMode requested, uint32_t width, uint32_t height )
{
	if( requested != TaaMode::Auto )
		return requested;
	const uint32_t shortAxis = std::min( width, height );
	return shortAxis >= 2160 ? TaaMode::Off : ( shortAxis >= 1800 ? TaaMode::Medium : TaaMode::High );
}

int TaaModeApiValue( TaaMode value )
{
	return value == TaaMode::Low ? 1 : ( value == TaaMode::Medium ? 2 : ( value == TaaMode::High ? 3 : 0 ) );
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

std::string EngineModeName( EngineMode value )
{
	static const char* names[] = { "auto", "off", "authored" };
	return names[static_cast<int>( value )];
}

bool ParseEngineMode( const std::string& value, EngineMode& result )
{
	const std::string normalized = ToLower( value );
	for( EngineMode candidate : { EngineMode::Auto, EngineMode::Off, EngineMode::Authored } )
	{
		if( normalized == EngineModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string EngineViewName( EngineView value )
{
	static const char* names[] = { "all", "plumes", "glows", "trails", "lights" };
	return names[static_cast<int>( value )];
}

bool ParseEngineView( const std::string& value, EngineView& result )
{
	const std::string normalized = ToLower( value );
	for( EngineView candidate :
		 { EngineView::All, EngineView::Plumes, EngineView::Glows, EngineView::Trails, EngineView::Lights } )
	{
		if( normalized == EngineViewName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string VolumetricModeName( VolumetricMode value )
{
	static const char* names[] = { "auto", "off", "silk", "froxel", "all" };
	return names[static_cast<int>( value )];
}

bool ParseVolumetricMode( const std::string& value, VolumetricMode& result )
{
	const std::string normalized = ToLower( value );
	for( VolumetricMode candidate : { VolumetricMode::Auto,
									  VolumetricMode::Off,
									  VolumetricMode::Silk,
									  VolumetricMode::Froxel,
									  VolumetricMode::All } )
	{
		if( normalized == VolumetricModeName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

std::string VolumetricQualityName( VolumetricQuality value )
{
	static const char* names[] = { "auto", "low", "medium", "high", "ultra" };
	return names[static_cast<int>( value )];
}

bool ParseVolumetricQuality( const std::string& value, VolumetricQuality& result )
{
	const std::string normalized = ToLower( value );
	for( VolumetricQuality candidate : { VolumetricQuality::Auto,
										 VolumetricQuality::Low,
										 VolumetricQuality::Medium,
										 VolumetricQuality::High,
										 VolumetricQuality::Ultra } )
	{
		if( normalized == VolumetricQualityName( candidate ) )
		{
			result = candidate;
			return true;
		}
	}
	return false;
}

int VolumetricModeApiValue( VolumetricMode value )
{
	switch( value )
	{
	case VolumetricMode::Silk:
		return 1;
	case VolumetricMode::Froxel:
		return 2;
	case VolumetricMode::All:
		return 3;
	case VolumetricMode::Auto:
	case VolumetricMode::Off:
		return 0;
	}
	return 0;
}

int VolumetricQualityApiValue( VolumetricQuality value )
{
	switch( value )
	{
	case VolumetricQuality::Low:
		return 0;
	case VolumetricQuality::Medium:
		return 1;
	case VolumetricQuality::Ultra:
		return 3;
	case VolumetricQuality::Auto:
	case VolumetricQuality::High:
		return 2;
	}
	return 2;
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
	static const char* names[] = { "auto", "off", "low", "high", "raytraced" };
	return names[static_cast<int>( shadows )];
}

bool ParseShadows( const std::string& value, Shadows& shadows )
{
	const std::string normalized = ToLower( value );
	for( int candidate = 0; candidate <= static_cast<int>( Shadows::Raytraced ); ++candidate )
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

std::string SolarIlluminationName( SolarIllumination illumination )
{
	static const char* names[] = { "frozen", "authored", "off" };
	return names[static_cast<int>( illumination )];
}

bool ParseSolarIllumination( const std::string& value, SolarIllumination& illumination )
{
	const std::string normalized = ToLower( value );
	for( SolarIllumination candidate :
		 { SolarIllumination::Frozen, SolarIllumination::Authored, SolarIllumination::Off } )
	{
		if( normalized == SolarIlluminationName( candidate ) )
		{
			illumination = candidate;
			return true;
		}
	}
	return false;
}

std::string SolarIlluminationViewName( SolarIlluminationView view )
{
	switch( view )
	{
	case SolarIlluminationView::Unspecified:
		return "unspecified";
	case SolarIlluminationView::NearSunHull:
		return "near-sun-hull";
	case SolarIlluminationView::GateHull:
		return "gate-hull";
	case SolarIlluminationView::PlanetDay:
		return "planet-day";
	case SolarIlluminationView::PlanetLimb:
		return "planet-limb";
	case SolarIlluminationView::PlanetEclipse:
		return "planet-eclipse";
	}
	return "invalid";
}

bool ParseSolarIlluminationView( const std::string& value, SolarIlluminationView& view )
{
	const std::string normalized = ToLower( value );
	for( SolarIlluminationView candidate :
		 { SolarIlluminationView::NearSunHull, SolarIlluminationView::GateHull,
			 SolarIlluminationView::PlanetDay, SolarIlluminationView::PlanetLimb,
			 SolarIlluminationView::PlanetEclipse } )
	{
		if( normalized == SolarIlluminationViewName( candidate ) )
		{
			view = candidate;
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
	case SunEffects::SunFlares:
		return "sun-flares";
	case SunEffects::LensFlare:
		return "lens-flare";
	}
	return "unknown";
}

bool ParseSunEffects( const std::string& value, SunEffects& effects )
{
	const std::string normalized = ToLower( value );
	const SunEffects values[] = {
		SunEffects::Auto,
		SunEffects::Off,
		SunEffects::Flare,
		SunEffects::GodRays,
		SunEffects::All,
		SunEffects::SunFlares,
		SunEffects::LensFlare,
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

bool ParseSolarEnvironment( const std::string& value, SolarEnvironment& environment )
{
	const std::string normalized = ToLower( value );
	if( normalized == "off" )
		environment = SolarEnvironment::Off;
	else if( normalized == "fog" )
		environment = SolarEnvironment::Fog;
	else if( normalized == "finish" )
		environment = SolarEnvironment::Finish;
	else if( normalized == "all" )
		environment = SolarEnvironment::All;
	else
		return false;
	return true;
}

bool ParseSolarEnvironmentDistance( const std::string& value, SolarEnvironmentDistance& distance )
{
	const std::string normalized = ToLower( value );
	if( normalized == "canonical" )
		distance = SolarEnvironmentDistance::Canonical;
	else if( normalized == "inside-boundary" )
		distance = SolarEnvironmentDistance::InsideBoundary;
	else if( normalized == "outside-boundary" )
		distance = SolarEnvironmentDistance::OutsideBoundary;
	else if( normalized == "exit-boundary" )
		distance = SolarEnvironmentDistance::ExitBoundary;
	else
		return false;
	return true;
}

bool ParseAuthoredToggle( const std::string& value, AuthoredToggle& toggle )
{
	const std::string normalized = ToLower( value );
	if( normalized == "authored" )
		toggle = AuthoredToggle::Authored;
	else if( normalized == "off" )
		toggle = AuthoredToggle::Off;
	else
		return false;
	return true;
}

bool ParseSolarOcclusionView( const std::string& value, SolarOcclusionView& view )
{
	const std::string normalized = ToLower( value );
	if( normalized == "unobscured" )
		view = SolarOcclusionView::Unobscured;
	else if( normalized == "hull-partial" )
		view = SolarOcclusionView::HullPartial;
	else if( normalized == "hull-total" )
		view = SolarOcclusionView::HullTotal;
	else if( normalized == "planet-limb" )
		view = SolarOcclusionView::PlanetLimb;
	else if( normalized == "planet-eclipse" )
		view = SolarOcclusionView::PlanetEclipse;
	else if( normalized == "hull-sweep" )
		view = SolarOcclusionView::HullSweep;
	else if( normalized == "planet-sweep" )
		view = SolarOcclusionView::PlanetSweep;
	else
		return false;
	return true;
}

std::string SunBodyLayersName( SunBodyLayers layers )
{
	switch( layers )
	{
	case SunBodyLayers::Off:
		return "off";
	case SunBodyLayers::Surface:
		return "surface";
	case SunBodyLayers::OuterBeams:
		return "outer-beams";
	case SunBodyLayers::EdgeClouds:
		return "edge-clouds";
	case SunBodyLayers::Corona:
		return "corona";
	case SunBodyLayers::All:
		return "all";
	}
	return "unknown";
}

bool ParseSunBodyLayers( const std::string& value, SunBodyLayers& layers )
{
	const std::string normalized = ToLower( value );
	const SunBodyLayers values[] = {
		SunBodyLayers::Off,        SunBodyLayers::Surface, SunBodyLayers::OuterBeams,
		SunBodyLayers::EdgeClouds, SunBodyLayers::Corona,  SunBodyLayers::All,
	};
	for( SunBodyLayers candidate : values )
	{
		if( normalized == SunBodyLayersName( candidate ) )
		{
			layers = candidate;
			return true;
		}
	}
	return false;
}

std::string SunHighLayersName( SunHighLayers layers )
{
	switch( layers )
	{
	case SunHighLayers::Off:
		return "off";
	case SunHighLayers::Whisps:
		return "whisps";
	case SunHighLayers::HugeDefinition:
		return "huge-definition";
	case SunHighLayers::Uber1:
		return "uber-1";
	case SunHighLayers::Uber2:
		return "uber-2";
	case SunHighLayers::RingsTop:
		return "rings-top";
	case SunHighLayers::RingsBottom:
		return "rings-bottom";
	case SunHighLayers::Rings:
		return "rings";
	case SunHighLayers::Pillar1:
		return "pillar-1";
	case SunHighLayers::Pillar2:
		return "pillar-2";
	case SunHighLayers::ParticleScaler:
		return "particle-scaler";
	case SunHighLayers::Particles:
		return "particles";
	case SunHighLayers::All:
		return "all";
	}
	return "unknown";
}

bool ParseSunHighLayers( const std::string& value, SunHighLayers& layers )
{
	const std::string normalized = ToLower( value );
	const SunHighLayers values[] = {
		SunHighLayers::Off, SunHighLayers::Whisps, SunHighLayers::HugeDefinition,
		SunHighLayers::Uber1, SunHighLayers::Uber2, SunHighLayers::RingsTop,
		SunHighLayers::RingsBottom, SunHighLayers::Rings, SunHighLayers::Pillar1,
		SunHighLayers::Pillar2, SunHighLayers::ParticleScaler, SunHighLayers::Particles,
		SunHighLayers::All,
	};
	for( SunHighLayers candidate : values )
	{
		if( normalized == SunHighLayersName( candidate ) )
		{
			layers = candidate;
			return true;
		}
	}
	return false;
}

std::string SolarParticlePrewarmName( SolarParticlePrewarm prewarm )
{
	return prewarm == SolarParticlePrewarm::NaturalFirstEmission ? "natural-first-emission" : "none";
}

bool ParseSolarParticlePrewarm( const std::string& value, SolarParticlePrewarm& prewarm )
{
	const std::string normalized = ToLower( value );
	if( normalized == "none" )
	{
		prewarm = SolarParticlePrewarm::None;
		return true;
	}
	if( normalized == "natural-first-emission" )
	{
		prewarm = SolarParticlePrewarm::NaturalFirstEmission;
		return true;
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
	case RenderProduct::VolumeSlices:
		return kCaptureVolumeSlices;
	case RenderProduct::FroxelFog:
		return kCaptureFroxelFog;
	case RenderProduct::MieEnvironment:
		return kCaptureMieEnvironment;
	case RenderProduct::Velocity:
		return kCaptureVelocity;
	case RenderProduct::TaaInput:
		return kCaptureTaaInput;
	case RenderProduct::TaaOutput:
		return kCaptureTaaOutput;
	case RenderProduct::TaaCooldown:
		return kCaptureTaaCooldown;
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
	case Shadows::Raytraced:
		return 3;
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
	case RenderProduct::VolumeSlices:
		return "source=VolumetricSlices type=2d-array format=R16G16B16A16_FLOAT layers=4 visualization=2x2-atlas gpuVisualization=true";
	case RenderProduct::FroxelFog:
		return "source=FroxelFog type=3d format=R11G11B10_FLOAT visualization=depth-projection gpuVisualization=true";
	case RenderProduct::MieEnvironment:
		return "source=MieEnvironmentMap type=cube format=R32G32B32A32_FLOAT layout=3x2 gpuVisualization=true";
	case RenderProduct::Velocity:
		return "source=VelocityMap format=R16G16_FLOAT units=pixels visualization=direction-magnitude gpuVisualization=true";
	case RenderProduct::TaaInput:
		return "source=PreTaaColor format=R16G16B16A16_FLOAT visualization=reinhard-plus-gamma gpuVisualization=true";
	case RenderProduct::TaaOutput:
		return "source=PostTaaColor format=R16G16B16A16_UNORM visualization=reinhard-plus-gamma gpuVisualization=true";
	case RenderProduct::TaaCooldown:
		return "source=TaaCooldownMap format=R32_UINT visualization=normalized-cooldown gpuVisualization=true";
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

bool ParseCaptureFrames( const std::string& text, std::vector<int>& frames )
{
	if( text.empty() )
	{
		return false;
	}
	std::vector<int> parsedFrames;
	size_t start = 0;
	while( start < text.size() )
	{
		const size_t comma = text.find( ',', start );
		const size_t end = comma == std::string::npos ? text.size() : comma;
		if( end == start )
		{
			return false;
		}
		const std::string token = text.substr( start, end - start );
		if( token.find_first_not_of( "0123456789" ) != std::string::npos )
		{
			return false;
		}
		char* parseEnd = nullptr;
		const unsigned long long parsed = std::strtoull( token.c_str(), &parseEnd, 10 );
		if( !parseEnd || *parseEnd != '\0' || parsed > static_cast<unsigned long long>( INT32_MAX ) )
		{
			return false;
		}
		parsedFrames.push_back( static_cast<int>( parsed ) );
		if( comma == std::string::npos )
		{
			break;
		}
		start = comma + 1;
		if( start == text.size() )
		{
			return false;
		}
	}
	std::sort( parsedFrames.begin(), parsedFrames.end() );
	if( std::adjacent_find( parsedFrames.begin(), parsedFrames.end() ) != parsedFrames.end() )
	{
		return false;
	}
	frames = std::move( parsedFrames );
	return !frames.empty();
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

#if defined( TRINITY_FROXEL_INCIDENT_LAB )
bool RunFroxelLabTool( NSArray<NSString*>* arguments )
{
	NSTask* task = [NSTask new];
	task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/python3"];
	NSMutableArray<NSString*>* completeArguments =
		[NSMutableArray arrayWithObject:[NSString stringWithUTF8String:TRINITY_FROXEL_LAB_TOOL]];
	[completeArguments addObjectsFromArray:arguments];
	task.arguments = completeArguments;
	NSPipe* output = [NSPipe pipe];
	task.standardOutput = output;
	task.standardError = output;
	NSError* error = nil;
	if( ![task launchAndReturnError:&error] )
	{
		std::cerr << "Failed to launch the froxel lab authorization tool: " << error.localizedDescription.UTF8String
				  << "\n";
		return false;
	}
	[task waitUntilExit];
	NSData* outputData = [output.fileHandleForReading readDataToEndOfFile];
	if( task.terminationStatus != 0 )
	{
		NSString* detail = [[NSString alloc] initWithData:outputData encoding:NSUTF8StringEncoding];
		std::cerr << "Froxel lab authorization failed: " << ( detail.UTF8String ?: "unknown error" ) << "\n";
		return false;
	}
	return true;
}

bool ValidateFroxelLabAuthorization( const Options& options )
{
	const char* acknowledgement = std::getenv( "TRINITY_FROXEL_GPU_LAB" );
	if( !options.clientKernels || options.froxelLabLedger.empty() || !acknowledgement ||
		std::strcmp( acknowledgement, "I_ACKNOWLEDGE_GPU_RESET_RISK" ) != 0 )
	{
		std::cerr << "The incident froxel lab requires --client-kernels, --froxel-lab-ledger, and the "
					 "GPU reset-risk environment acknowledgement\n";
		return false;
	}
	return RunFroxelLabTool( @[
		@"_validate-authorization",
		@"--ledger",
		[NSString stringWithUTF8String:options.froxelLabLedger.c_str()],
	] );
}

bool WriteAndSyncIncidentPreflight( const Options& options )
{
	NSString* ledger = [NSString stringWithUTF8String:options.froxelLabLedger.c_str()];
	NSString* path =
		[ledger.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"submission-preflight.json"];
	NSDictionary* record = @{
		@"schemaVersion": @2,
		@"kernelSet": @"client-scene",
		@"stage": @"incident-equivalent-chain",
		@"dimensions": @{ @"width": @( options.windowWidth ), @"height": @( options.windowHeight ) },
		@"volumetrics": [NSString stringWithUTF8String:VolumetricModeName( options.resolvedVolumetrics ).c_str()],
		@"quality": [NSString stringWithUTF8String:VolumetricQualityName( options.resolvedVolumetricQuality ).c_str()],
		@"froxelTemporal": options.froxelTemporal ? @"on" : @"off",
		@"taa": [NSString stringWithUTF8String:TaaModeName( options.resolvedTaa ).c_str()],
		@"sceneFixture": @"new-eden",
		@"resourcePreparationComplete": @YES,
	};
	NSError* error = nil;
	NSData* data = [NSJSONSerialization dataWithJSONObject:record
												   options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys
													 error:&error];
	if( !data || ![data writeToFile:path options:NSDataWritingAtomic error:&error] )
	{
		std::cerr << "Failed to persist incident submission preflight: " << error.localizedDescription.UTF8String
				  << "\n";
		return false;
	}
	const int file = open( path.fileSystemRepresentation, O_RDONLY );
	if( file < 0 || fsync( file ) != 0 )
	{
		if( file >= 0 )
			close( file );
		std::cerr << "Failed to fsync incident submission preflight\n";
		return false;
	}
	close( file );
	const int directory = open( path.stringByDeletingLastPathComponent.fileSystemRepresentation, O_RDONLY );
	if( directory >= 0 )
	{
		fsync( directory );
		close( directory );
	}
	return RunFroxelLabTool( @[
		@"_mark-submitted",
		@"--ledger",
		ledger,
		@"--preflight",
		path,
	] );
}

bool WriteAndSyncIncidentReport( const Options& options, void* probe )
{
	const char* json = TrinityStandaloneProbeGetIncidentDiagnosticsJson( probe );
	if( !json )
	{
		std::cerr << "Incident diagnostics were unavailable after rendering\n";
		return false;
	}
	NSData* data = [NSData dataWithBytes:json length:std::strlen( json )];
	NSError* error = nil;
	if( ![NSJSONSerialization JSONObjectWithData:data options:0 error:&error] )
	{
		std::cerr << "Incident diagnostics JSON is invalid: " << error.localizedDescription.UTF8String << "\n";
		return false;
	}
	NSString* ledger = [NSString stringWithUTF8String:options.froxelLabLedger.c_str()];
	NSString* path =
		[ledger.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"incident-report.json"];
	if( ![data writeToFile:path options:NSDataWritingAtomic error:&error] )
	{
		std::cerr << "Failed to persist incident diagnostics: " << error.localizedDescription.UTF8String << "\n";
		return false;
	}
	const int file = open( path.fileSystemRepresentation, O_RDONLY );
	if( file < 0 || fsync( file ) != 0 )
	{
		if( file >= 0 )
			close( file );
		std::cerr << "Failed to fsync incident diagnostics\n";
		return false;
	}
	close( file );
	const int directory = open( path.stringByDeletingLastPathComponent.fileSystemRepresentation, O_RDONLY );
	if( directory >= 0 )
	{
		fsync( directory );
		close( directory );
	}
	return true;
}
#endif

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
		<< "       [--render-product window|color|depth|normal|shadow|shadow-atlas|local-shadow-atlas|ao|bent-normal|reflection|hdr-composite|post-tonemap|bloom|final-postprocess|distortion|volume-slices|froxel-fog|mie-environment|velocity|taa-input|taa-output|taa-cooldown|all]\n"
		<< "       [--shadows auto|off|low|high|raytraced] [--ao auto|off|low|medium|high]\n"
		<< "       [--ao-method cortao|cacao]\n"
		<< "       [--camera-view model|celestials|planet]\n"
		<< "       [--composition system|cinematic] [--planet-layers surface|atmosphere|clouds|all]\n"
		<< "       [--sun-effects auto|off|flare|sun-flares|lens-flare|god-rays|all]\n"
		<< "       [--solar-environment off|fog|finish|all] [--solar-environment-distance canonical|inside-boundary|outside-boundary|exit-boundary]\n"
		<< "       [--scene-distance-fog authored|off] [--solar-desaturate authored|off]\n"
		<< "       [--solar-occlusion-view unobscured|hull-partial|hull-total|planet-limb|planet-eclipse|hull-sweep|planet-sweep]\n"
		<< "       [--solar-optics-report PATH]\n"
		<< "       [--solar-illumination frozen|authored|off] [--solar-illumination-report PATH]\n"
		<< "       [--solar-illumination-view near-sun-hull|gate-hull|planet-day|planet-limb|planet-eclipse]\n"
		<< "       [--sun-body-layers off|surface|outer-beams|edge-clouds|corona|all]\n"
		<< "       [--sun-high-layers off|whisps|huge-definition|uber-1|uber-2|rings-top|rings-bottom|rings|pillar-1|pillar-2|particle-scaler|particles|all]\n"
		<< "       [--planet-cloud-date YYYY-MM-DD|today] [--background-capture] [--deterministic-evidence]\n"
		<< "       [--dynamic-exposure auto|off|client] [--validate-exposure-tone]\n"
		<< "       [--bloom auto|off|client] [--film-grain auto|off|client] [--validate-post-finish]\n"
		<< "       [--taa auto|off|low|medium|high] [--taa-debug off|motion-vectors|early-out]\n"
		<< "       [--motion static|camera|object|combined] [--validate-temporal]\n"
		<< "       [--ballpark off|static|goto|orbit|warp|approach] [--warp-target planet|evegate] [--journey-planet-finale]\n"
		<< "       [--ballpark-frame ego|observer|chase]\n"
		<< "       [--orbit-solver legacy|new] [--orbit-range FLOAT]\n"
		<< "       [--validate-ballpark] [--validate-ballpark-motion] [--validate-ballpark-orbit]\n"
		<< "       [--validate-ballpark-warp] [--validate-ballpark-approach] [--ballpark-log PATH] [--capture-every N] [--capture-start-frame N] [--capture-frames CSV]\n"
		<< "       [--celestial-ballpark off|natural] [--validate-celestial-ballpark] [--celestial-log PATH]\n"
		<< "       [--eve-gate-approach FRAME] [--eve-gate off|authored] [--validate-eve-gate]\n"
		<< "       [--warp-tunnel off|authored]\n"
		<< "       [--celestial-anchor stargate|evegate] [--eve-gate-ratio FLOAT] [--eve-gate-travel orbit|direct]\n"
		<< "       [--validate-chase-camera]\n"
		<< "       [--temporal-test contract|velocity|edges|silk|trails|integrated]\n"
		<< "       [--distortion auto|off|authored] [--validate-distortion]\n"
		<< "       [--engines auto|off|authored] [--engine-view all|plumes|glows|trails|lights]\n"
		<< "       [--engine-throttle FLOAT] [--validate-engines]\n"
		<< "       [--volumetrics auto|off|silk|froxel|all] [--volumetric-quality auto|low|medium|high|ultra]\n"
		<< "       [--volumetric-seed N] [--enable-froxels] [--froxel-temporal on|off] [--validate-volumetrics]\n"
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
		<< "       [--client-kernels --froxel-lab-ledger PATH]\n"
#endif
		<< "       [--exposure-sequence none|dark-to-bright|bright-to-dark] [--exposure-hold N]\n"
		<< "       [--validate-composition] [--frame-pacing-check] [--timing-warmup N]\n"
		<< "       [--material-view lit|basecolor|normal|roughness|material|glow|d|mask|p3]\n"
		<< "       [--capture-prefix PATH] [--inspect-client-assets REPORT.md] [--solar-audit-report REPORT.json]\n"
		<< "       [--solar-body-report REPORT.json] [--solar-high-report REPORT.json]\n"
		<< "       [--solar-particle-seed N] [--solar-particle-prewarm none|natural-first-emission]\n"
		<< "       [--solar-high-warmup N]\n";
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
		else if( arg == "--taa" )
		{
			if( ++i >= argc || !ParseTaaMode( argv[i], options.taa ) )
				return false;
		}
		else if( arg == "--taa-debug" )
		{
			if( ++i >= argc || !ParseTaaDebug( argv[i], options.taaDebug ) )
				return false;
		}
		else if( arg == "--motion" )
		{
			if( ++i >= argc || !ParseMotionMode( argv[i], options.motion ) )
				return false;
		}
		else if( arg == "--ballpark" )
		{
			if( ++i >= argc || !ParseBallparkMode( argv[i], options.ballpark ) )
				return false;
		}
		else if( arg == "--warp-target" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = ToLower( argv[i] );
			if( value == "planet" )
				options.warpTarget = WarpTarget::Planet;
			else if( value == "evegate" || value == "eve-gate" )
				options.warpTarget = WarpTarget::EveGate;
			else
				return false;
		}
		else if( arg == "--journey-planet-finale" )
		{
			options.journeyPlanetFinale = true;
		}
		else if( arg == "--ballpark-frame" )
		{
			if( ++i >= argc || !ParseBallparkFrame( argv[i], options.ballparkFrame ) )
				return false;
		}
		else if( arg == "--orbit-solver" )
		{
			if( ++i >= argc || !ParseOrbitSolver( argv[i], options.orbitSolver ) )
				return false;
		}
		else if( arg == "--orbit-range" )
		{
			if( ++i >= argc )
				return false;
			char* end = nullptr;
			options.orbitRange = std::strtof( argv[i], &end );
			if( !end || *end != '\0' || !std::isfinite( options.orbitRange ) || options.orbitRange < 0.0f )
				return false;
		}
		else if( arg == "--ballpark-log" )
		{
			if( ++i >= argc )
				return false;
			options.ballparkLogPath = argv[i];
		}
		else if( arg == "--validate-ballpark" )
		{
			options.validateBallpark = true;
		}
		else if( arg == "--validate-ballpark-motion" )
		{
			options.validateBallparkMotion = true;
		}
		else if( arg == "--validate-ballpark-orbit" )
		{
			options.validateBallparkOrbit = true;
		}
		else if( arg == "--validate-ballpark-warp" )
		{
			options.validateBallparkWarp = true;
		}
		else if( arg == "--validate-ballpark-approach" )
		{
			options.validateBallparkApproach = true;
		}
		else if( arg == "--capture-every" )
		{
			if( ++i >= argc )
				return false;
			options.captureEvery = std::atoi( argv[i] );
			if( options.captureEvery < 0 )
				return false;
		}
		else if( arg == "--capture-frames" )
		{
			if( ++i >= argc || !ParseCaptureFrames( argv[i], options.captureFrames ) )
			{
				return false;
			}
		}
		else if( arg == "--capture-start-frame" )
		{
			if( ++i >= argc )
				return false;
			options.captureStartFrame = std::atoi( argv[i] );
			if( options.captureStartFrame < 0 )
				return false;
		}
		else if( arg == "--validate-chase-camera" )
		{
			options.validateChaseCamera = true;
		}
		else if( arg == "--celestial-ballpark" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = argv[i];
			if( value == "off" )
				options.celestialBallpark = CelestialBallparkMode::Off;
			else if( value == "natural" )
				options.celestialBallpark = CelestialBallparkMode::Natural;
			else
				return false;
		}
		else if( arg == "--celestial-log" )
		{
			if( ++i >= argc )
				return false;
			options.celestialLogPath = argv[i];
		}
		else if( arg == "--validate-celestial-ballpark" )
		{
			options.validateCelestialBallpark = true;
		}
		else if( arg == "--eve-gate-approach" )
		{
			if( ++i >= argc )
				return false;
			char* end = nullptr;
			const unsigned long long parsed = std::strtoull( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed == 0 )
				return false;
			options.eveGateApproachFrame = parsed;
		}
		else if( arg == "--eve-gate" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = argv[i];
			if( value == "off" )
				options.eveGate = 0;
			else if( value == "authored" )
				options.eveGate = 1;
			else
				return false;
		}
		else if( arg == "--warp-tunnel" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = argv[i];
			if( value == "off" )
				options.warpTunnel = 0;
			else if( value == "authored" )
				options.warpTunnel = 1;
			else
				return false;
		}
		else if( arg == "--validate-eve-gate" )
		{
			options.validateEveGate = true;
		}
		else if( arg == "--eve-gate-travel" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = argv[i];
			if( value == "orbit" )
				options.eveGateTravel = 0;
			else if( value == "direct" )
				options.eveGateTravel = 1;
			else
				return false;
		}
		else if( arg == "--eve-gate-ratio" )
		{
			if( ++i >= argc )
				return false;
			char* end = nullptr;
			options.eveGateRatio = std::strtof( argv[i], &end );
			if( !end || *end != '\0' || !std::isfinite( options.eveGateRatio ) || options.eveGateRatio < 0.0f )
				return false;
		}
		else if( arg == "--celestial-anchor" )
		{
			if( ++i >= argc )
				return false;
			const std::string value = argv[i];
			if( value == "stargate" )
				options.celestialAnchor = 0;
			else if( value == "evegate" )
				options.celestialAnchor = 1;
			else
				return false;
		}
		else if( arg == "--temporal-test" )
		{
			if( ++i >= argc || !ParseTemporalTest( argv[i], options.temporalTest ) )
				return false;
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
		else if( arg == "--engines" )
		{
			if( ++i >= argc || !ParseEngineMode( argv[i], options.engines ) )
			{
				return false;
			}
		}
		else if( arg == "--engine-view" )
		{
			if( ++i >= argc || !ParseEngineView( argv[i], options.engineView ) )
			{
				return false;
			}
		}
		else if( arg == "--engine-throttle" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			const float parsed = std::strtof( argv[i], &end );
			if( !end || *end != '\0' || !std::isfinite( parsed ) || parsed < 0.0f || parsed > 1.0f )
			{
				return false;
			}
			options.engineThrottle = parsed;
		}
		else if( arg == "--volumetrics" )
		{
			if( ++i >= argc || !ParseVolumetricMode( argv[i], options.volumetrics ) )
			{
				return false;
			}
		}
		else if( arg == "--volumetric-quality" )
		{
			if( ++i >= argc || !ParseVolumetricQuality( argv[i], options.volumetricQuality ) )
			{
				return false;
			}
		}
		else if( arg == "--volumetric-seed" )
		{
			if( ++i >= argc || !ParseUnsigned( argv[i], options.volumetricSeed ) )
			{
				return false;
			}
		}
		else if( arg == "--enable-froxels" )
		{
			options.enableFroxels = true;
		}
		else if( arg == "--froxel-temporal" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			const std::string value = argv[i];
			if( value == "on" )
			{
				options.froxelTemporal = true;
			}
			else if( value == "off" )
			{
				options.froxelTemporal = false;
			}
			else
			{
				return false;
			}
		}
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
		else if( arg == "--client-kernels" )
		{
			options.clientKernels = true;
		}
		else if( arg == "--froxel-lab-ledger" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.froxelLabLedger = argv[i];
		}
#endif
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
		else if( arg == "--validate-temporal" )
		{
			options.validateTemporal = true;
		}
		else if( arg == "--validate-post-finish" )
		{
			options.validatePostFinish = true;
		}
		else if( arg == "--validate-distortion" )
		{
			options.validateDistortion = true;
		}
		else if( arg == "--validate-engines" )
		{
			options.validateEngines = true;
		}
		else if( arg == "--validate-volumetrics" )
		{
			options.validateVolumetrics = true;
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
		else if( arg == "--deterministic-evidence" )
		{
			options.deterministicEvidence = true;
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
		else if( arg == "--solar-environment" )
		{
			if( ++i >= argc || !ParseSolarEnvironment( argv[i], options.solarEnvironment ) )
			{
				return false;
			}
		}
		else if( arg == "--solar-environment-distance" )
		{
			if( ++i >= argc ||
				!ParseSolarEnvironmentDistance( argv[i], options.solarEnvironmentDistance ) )
			{
				return false;
			}
		}
		else if( arg == "--scene-distance-fog" )
		{
			if( ++i >= argc || !ParseAuthoredToggle( argv[i], options.sceneDistanceFog ) )
			{
				return false;
			}
		}
		else if( arg == "--solar-desaturate" )
		{
			if( ++i >= argc || !ParseAuthoredToggle( argv[i], options.solarDesaturate ) )
			{
				return false;
			}
		}
		else if( arg == "--solar-occlusion-view" )
		{
			if( ++i >= argc || !ParseSolarOcclusionView( argv[i], options.solarOcclusionView ) )
			{
				return false;
			}
		}
		else if( arg == "--solar-illumination" )
		{
			if( ++i >= argc || !ParseSolarIllumination( argv[i], options.solarIllumination ) )
			{
				return false;
			}
			options.solarIlluminationExplicit = true;
		}
		else if( arg == "--solar-illumination-view" )
		{
			if( ++i >= argc ||
				!ParseSolarIlluminationView( argv[i], options.solarIlluminationView ) )
			{
				return false;
			}
			options.solarIlluminationViewExplicit = true;
		}
		else if( arg == "--sun-body-layers" )
		{
			if( ++i >= argc || !ParseSunBodyLayers( argv[i], options.sunBodyLayers ) )
			{
				return false;
			}
		}
		else if( arg == "--sun-high-layers" )
		{
			if( ++i >= argc || !ParseSunHighLayers( argv[i], options.sunHighLayers ) )
			{
				return false;
			}
			options.sunHighLayersExplicit = true;
		}
		else if( arg == "--solar-particle-seed" )
		{
			if( ++i >= argc || !ParseUnsigned( argv[i], options.solarParticleSeed ) ||
				options.solarParticleSeed >= 0x7FFFFFFFu )
			{
				return false;
			}
			options.solarParticleSeedExplicit = true;
		}
		else if( arg == "--solar-particle-prewarm" )
		{
			if( ++i >= argc || !ParseSolarParticlePrewarm( argv[i], options.solarParticlePrewarm ) )
			{
				return false;
			}
		}
		else if( arg == "--solar-high-warmup" )
		{
			if( ++i >= argc || !ParseUnsigned( argv[i], options.solarHighWarmup ) ||
				options.solarHighWarmup == 0 )
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
		else if( arg == "--solar-audit-report" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.solarAuditReportPath = argv[i];
		}
		else if( arg == "--solar-body-report" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.solarBodyReportPath = argv[i];
		}
		else if( arg == "--solar-high-report" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.solarHighReportPath = argv[i];
		}
		else if( arg == "--solar-optics-report" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.solarOpticsReportPath = argv[i];
		}
		else if( arg == "--solar-illumination-report" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.solarIlluminationReportPath = argv[i];
		}
		else
		{
			std::cerr << "Unknown command-line option: " << arg << "\n";
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
	options.resolvedTaa = options.taa == TaaMode::Auto ?
		( options.qualityRung >= QualityRung::HdrPost && options.shaderTier == ShaderTier::High ?
			  ResolveClientTaa( options.taa, options.windowWidth, options.windowHeight ) :
			  TaaMode::Off ) :
		options.taa;
	if( options.resolvedTaa != TaaMode::Off &&
		( options.qualityRung < QualityRung::HdrPost || options.shaderTier != ShaderTier::High ) )
	{
		std::cerr << "TAA requires --quality-rung hdr-post or higher with --shader-tier high\n";
		return false;
	}
	if( options.taaDebug != TaaDebug::Off && options.resolvedTaa == TaaMode::Off )
	{
		std::cerr << "TAA debug output requires enabled TAA\n";
		return false;
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
	if( options.resolvedLocalShadows != LocalShadows::Off &&
		options.resolvedShadows == Shadows::Raytraced )
	{
		std::cerr << "Raytraced shadows require --local-shadows off\n";
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
	const bool distortionCompatible = options.qualityRung == QualityRung::HdrFinish && options.asset == "astero" &&
		options.materialMode == MaterialMode::EveV5 && options.shaderTier == ShaderTier::High &&
		( options.areaView == AreaView::All || options.areaView == AreaView::Distortion );
	options.resolvedDistortion = options.distortion == DistortionMode::Auto ?
		( distortionCompatible ? DistortionMode::Authored : DistortionMode::Off ) :
		options.distortion;
	if( options.resolvedDistortion == DistortionMode::Authored && !distortionCompatible )
	{
		std::cerr << "Authored distortion requires high-tier Astero eve-v5 rendering at hdr-finish\n";
		return false;
	}
	const bool engineCompatible = options.qualityRung >= QualityRung::Model && options.asset == "astero" &&
		options.materialMode == MaterialMode::EveV5 && options.materialView == MaterialView::Lit &&
		options.areaView == AreaView::All && options.shaderTier == ShaderTier::High;
	options.resolvedEngines = options.engines == EngineMode::Auto ?
		( engineCompatible && options.qualityRung == QualityRung::HdrFinish ? EngineMode::Authored : EngineMode::Off ) :
		options.engines;
	if( options.resolvedEngines == EngineMode::Authored && !engineCompatible )
	{
		std::cerr << "Authored engines require a lit, all-area, high-tier Astero eve-v5 render at model or higher\n";
		return false;
	}
	options.resolvedVolumetrics =
		options.volumetrics == VolumetricMode::Auto ? VolumetricMode::Off : options.volumetrics;
	options.resolvedVolumetricQuality =
		options.volumetricQuality == VolumetricQuality::Auto ? VolumetricQuality::High : options.volumetricQuality;
	if( options.resolvedVolumetrics == VolumetricMode::Froxel || options.resolvedVolumetrics == VolumetricMode::All )
	{
		if( !options.enableFroxels )
		{
			std::cerr << "Froxel volumetrics require the explicit --enable-froxels opt-in.\n";
			return false;
		}
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
		if( options.clientKernels && !ValidateFroxelLabAuthorization( options ) )
		{
			return false;
		}
#endif
	}
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
	else if( options.clientKernels || !options.froxelLabLedger.empty() )
	{
		std::cerr << "The incident-lab authorization flags may only be used with froxel or all volumetrics\n";
		return false;
	}
#endif
	if( options.resolvedVolumetrics != VolumetricMode::Off && options.qualityRung < QualityRung::Scene )
	{
		std::cerr << "Volumetric fixtures require the scene rung or higher\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::HdrComposite && options.qualityRung < QualityRung::HdrPost )
	{
		std::cerr << "The hdr-composite render product requires --quality-rung hdr-post or hdr-exposure\n";
		return false;
	}
	if( ( options.renderProduct == RenderProduct::TaaInput || options.renderProduct == RenderProduct::TaaOutput ||
		  options.renderProduct == RenderProduct::TaaCooldown ) &&
		options.resolvedTaa == TaaMode::Off )
	{
		std::cerr << "TAA render products require enabled TAA\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::Velocity && options.qualityRung < QualityRung::Model )
	{
		std::cerr << "Velocity capture requires the model rung or higher\n";
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
	if( options.renderProduct == RenderProduct::Distortion && options.resolvedDistortion != DistortionMode::Authored )
	{
		std::cerr << "The distortion render product requires authored distortion\n";
		return false;
	}
	if( options.renderProduct == RenderProduct::VolumeSlices && options.resolvedVolumetrics != VolumetricMode::Silk &&
		options.resolvedVolumetrics != VolumetricMode::All )
	{
		std::cerr << "The volume-slices render product requires silk or all volumetrics\n";
		return false;
	}
	if( ( options.renderProduct == RenderProduct::FroxelFog ||
		  options.renderProduct == RenderProduct::MieEnvironment ) &&
		options.resolvedVolumetrics != VolumetricMode::Froxel && options.resolvedVolumetrics != VolumetricMode::All )
	{
		std::cerr << "Froxel and Mie render products require froxel or all volumetrics\n";
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
	if( options.validateTemporal )
	{
		if( options.maxFrames <= 0 || options.capturePrefix.empty() || options.qualityRung < QualityRung::Model )
		{
			std::cerr << "--validate-temporal requires model or higher, finite frames, and --capture-prefix\n";
			return false;
		}
		const bool velocityTest = options.temporalTest == TemporalTest::Velocity;
		if( !velocityTest && ( options.qualityRung < QualityRung::HdrPost || options.resolvedTaa != TaaMode::High ) )
		{
			std::cerr << "This temporal test requires hdr-post or higher with resolved High TAA\n";
			return false;
		}
		const uint32_t frameCount = static_cast<uint32_t>( options.maxFrames );
		if( velocityTest && frameCount < ( options.motion == MotionMode::Static ? 180u : 240u ) )
		{
			std::cerr << "Velocity validation requires 180 static frames or at least 240 moving frames\n";
			return false;
		}
		if( options.temporalTest == TemporalTest::Edges &&
			( frameCount < 184 || options.motion != MotionMode::Static ) )
		{
			std::cerr << "Edge validation requires --motion static and at least 184 frames\n";
			return false;
		}
		if( ( options.temporalTest == TemporalTest::Silk || options.temporalTest == TemporalTest::Trails ) &&
			( frameCount < 300 || options.motion != MotionMode::Camera ) )
		{
			std::cerr << "Transient validation requires --motion camera and at least 300 frames\n";
			return false;
		}
		const bool isolatedEffectsOff = options.resolvedAttachments == Attachments::Off &&
			options.resolvedDecals == Decals::Off && options.localLights == LocalLights::Off &&
			options.resolvedLocalShadows == LocalShadows::Off && options.resolvedShadows == Shadows::Off &&
			options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.resolvedReflectionSource == ReflectionSource::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedSunEffects == SunEffects::Off;
		if( options.temporalTest == TemporalTest::Edges &&
			( !isolatedEffectsOff || options.resolvedVolumetrics != VolumetricMode::Off ||
			  options.resolvedEngines != EngineMode::Off ) )
		{
			std::cerr << "Edge validation requires isolated opaque rendering with scene effects disabled\n";
			return false;
		}
		if( options.temporalTest == TemporalTest::Silk &&
			( !isolatedEffectsOff || options.resolvedVolumetrics != VolumetricMode::Silk ||
			  options.resolvedEngines != EngineMode::Off ) )
		{
			std::cerr << "Silk validation requires isolated Silk with engines and other scene effects disabled\n";
			return false;
		}
		if( options.temporalTest == TemporalTest::Trails &&
			( !isolatedEffectsOff || options.resolvedVolumetrics != VolumetricMode::Off ||
			  options.resolvedEngines != EngineMode::Authored || options.engineView != EngineView::Trails ) )
		{
			std::cerr << "Trail validation requires isolated authored engines with --engine-view trails\n";
			return false;
		}
		if( options.temporalTest == TemporalTest::Integrated &&
			( frameCount < 1080 || options.motion != MotionMode::Camera ||
			  options.resolvedVolumetrics != VolumetricMode::Silk || options.resolvedEngines != EngineMode::Authored ||
			  options.resolvedDistortion != DistortionMode::Authored ||
			  !ValidateCanonicalCompositionOptions( options ) ) )
		{
			std::cerr << "Integrated temporal validation requires the 1080-frame canonical RC-13 profile\n";
			return false;
		}
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
		std::cerr
			<< "--validate-distortion requires at least --exposure-hold frames for settled final-color validation\n";
		return false;
	}
	if( options.validateDistortion && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.validateVolumetrics &&
		( options.resolvedVolumetrics != VolumetricMode::Silk ||
		  options.resolvedVolumetricQuality != VolumetricQuality::High ||
		  options.qualityRung != QualityRung::HdrFinish || options.maxFrames <= 0 || options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-volumetrics requires --volumetrics silk --volumetric-quality high, hdr-finish, "
					 "finite frames, and --capture-prefix\n";
		return false;
	}
	if( options.validateVolumetrics && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.validateEngines &&
		( options.resolvedEngines != EngineMode::Authored || options.engineView != EngineView::All ||
		  std::abs( options.engineThrottle - 1.0f ) > 0.000001f || options.qualityRung != QualityRung::HdrFinish ||
		  options.maxFrames <= 0 || static_cast<uint32_t>( options.maxFrames ) < options.exposureHold ||
		  options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-engines requires authored all-family engines at throttle 1, hdr-finish, "
					 "at least --exposure-hold frames, and --capture-prefix\n";
		return false;
	}
	if( options.validateEngines && !ValidateCanonicalCompositionOptions( options ) )
	{
		return false;
	}
	if( options.ballpark == BallparkMode::Static &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ||
		  options.qualityRung < QualityRung::Model ||
		  ( options.motion != MotionMode::Static && options.motion != MotionMode::Camera ) ) )
	{
		std::cerr << "Static Ballpark mode requires an Astero eve-v5 model render with static or camera motion\n";
		return false;
	}
	if( options.ballpark == BallparkMode::Goto &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ||
		  options.qualityRung < QualityRung::Model || options.motion != MotionMode::Static ) )
	{
		std::cerr << "GOTO Ballpark mode requires an Astero eve-v5 model render with sample motion static\n";
		return false;
	}
	if( options.ballpark == BallparkMode::Orbit &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ||
		  options.qualityRung < QualityRung::Model || options.motion != MotionMode::Static ) )
	{
		std::cerr << "ORBIT Ballpark mode requires an Astero eve-v5 model render with sample motion static\n";
		return false;
	}
	if( options.ballpark == BallparkMode::Warp &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ||
		  options.qualityRung < QualityRung::Model || options.motion != MotionMode::Static ) )
	{
		std::cerr << "WARP Ballpark mode requires an Astero eve-v5 model render with sample motion static\n";
		return false;
	}
	if( options.ballpark == BallparkMode::Approach &&
		( options.asset != "astero" || options.materialMode != MaterialMode::EveV5 ||
		  options.qualityRung < QualityRung::Model || options.motion != MotionMode::Static ) )
	{
		std::cerr << "APPROACH Ballpark mode requires an Astero eve-v5 model render with sample motion static\n";
		return false;
	}
	if( options.warpTarget == WarpTarget::EveGate &&
		( options.ballpark != BallparkMode::Warp || options.sceneFixture != SceneFixture::NewEden ||
		  options.composition != SceneComposition::System ||
		  options.celestialBallpark != CelestialBallparkMode::Natural || options.eveGate != 1 ||
		  options.celestialAnchor != 0 ) )
	{
		std::cerr << "--warp-target evegate requires --ballpark warp, the exact-system New Eden fixture, "
					 "natural celestials, the authored EVE Gate, and --celestial-anchor stargate\n";
		return false;
	}
	if( options.journeyPlanetFinale &&
		( options.warpTarget != WarpTarget::EveGate || options.ballpark != BallparkMode::Warp ||
		  options.ballparkFrame != BallparkFrame::Chase ||
		  options.sceneFixture != SceneFixture::NewEden || options.composition != SceneComposition::System ||
		  options.celestialBallpark != CelestialBallparkMode::Natural || options.eveGate != 1 ||
		  options.celestialAnchor != 0 || options.validateBallpark || options.validateBallparkMotion ||
		  options.validateBallparkOrbit || options.validateBallparkWarp || options.validateBallparkApproach ||
		  options.validateCelestialBallpark || options.validateChaseCamera || options.validateEveGate ) )
	{
		std::cerr << "--journey-planet-finale is a Tour-only extension of the New Eden EVE Gate warp/chase route\n";
		return false;
	}
	if( options.ballparkFrame != BallparkFrame::Ego && options.ballpark != BallparkMode::Goto &&
		options.ballpark != BallparkMode::Orbit && options.ballpark != BallparkMode::Warp &&
		options.ballpark != BallparkMode::Approach )
	{
		std::cerr << "Observer and chase Ballpark frames require --ballpark goto, orbit, warp, or approach\n";
		return false;
	}
	if( options.validateBallpark )
	{
		const bool canonical = options.ballpark == BallparkMode::Static && options.maxFrames == 180 &&
			options.qualityRung == QualityRung::HdrPost && options.motion == MotionMode::Static &&
			options.sceneFixture == SceneFixture::NewEden && options.composition == SceneComposition::Cinematic &&
			options.resolvedTaa == TaaMode::Off && options.resolvedDynamicExposure == DynamicExposure::Off &&
			options.resolvedBloom == PostFinishMode::Off && options.resolvedFilmGrain == PostFinishMode::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedVolumetrics == VolumetricMode::Off &&
			options.resolvedEngines == EngineMode::Off &&
			options.resolvedReflectionSource == ReflectionSource::Static &&
			options.resolvedAttachments == Attachments::Authored && options.resolvedDecals == Decals::Authored &&
			options.localLights == LocalLights::Authored && options.resolvedLocalShadows == LocalShadows::Off &&
			options.resolvedShadows == Shadows::Off && options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.planetLayers == PlanetLayers::All && options.resolvedSunEffects == SunEffects::Flare &&
			!options.ballparkLogPath.empty() && !options.capturePrefix.empty();
		if( !canonical )
		{
			std::cerr << "--validate-ballpark requires the 180-frame PL-10 hdr-post fixture: static camera, static "
						 "reflection, authored attachments/decals/lights/planet/flare, all temporal and finish effects "
						 "off, --ballpark-log, and --capture-prefix\n";
			return false;
		}
	}
	if( options.validateBallparkMotion )
	{
		if( options.ballparkFrame == BallparkFrame::Chase )
		{
			std::cerr
				<< "The PL-11A quantitative contract accepts ego or fixed-observer frames; chase is visual-only\n";
			return false;
		}
		const bool engineFixture = options.ballparkFrame == BallparkFrame::Observer ?
			options.resolvedEngines == EngineMode::Authored :
			options.resolvedEngines == EngineMode::Off;
		const bool canonical = options.ballpark == BallparkMode::Goto && options.maxFrames == 1200 &&
			options.qualityRung == QualityRung::HdrPost && options.motion == MotionMode::Static &&
			options.sceneFixture == SceneFixture::NewEden && options.composition == SceneComposition::Cinematic &&
			options.resolvedTaa == TaaMode::Off && options.resolvedDynamicExposure == DynamicExposure::Off &&
			options.resolvedBloom == PostFinishMode::Off && options.resolvedFilmGrain == PostFinishMode::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedVolumetrics == VolumetricMode::Off &&
			engineFixture && options.resolvedReflectionSource == ReflectionSource::Static &&
			options.resolvedAttachments == Attachments::Authored && options.resolvedDecals == Decals::Authored &&
			options.localLights == LocalLights::Authored && options.resolvedLocalShadows == LocalShadows::Off &&
			options.resolvedShadows == Shadows::Off && options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.planetLayers == PlanetLayers::All && options.resolvedSunEffects == SunEffects::Flare &&
			!options.ballparkLogPath.empty() && !options.capturePrefix.empty();
		if( !canonical )
		{
			std::cerr
				<< "--validate-ballpark-motion requires the 1200-frame PL-11A hdr-post fixture: goto/static "
				   "sample motion, static reflection, authored attachments/decals/lights/planet/flare, all temporal "
				   "and finish effects off, engines off for ego or authored for observer, --ballpark-log, and "
				   "--capture-prefix\n";
			return false;
		}
	}
	if( options.validateBallparkOrbit )
	{
		if( options.ballparkFrame == BallparkFrame::Chase )
		{
			std::cerr
				<< "The PL-11B quantitative contract accepts ego or fixed-observer frames; chase is visual-only\n";
			return false;
		}
		const bool engineFixture = options.ballparkFrame == BallparkFrame::Observer ?
			options.resolvedEngines == EngineMode::Authored :
			options.resolvedEngines == EngineMode::Off;
		const bool canonical = options.ballpark == BallparkMode::Orbit && options.orbitSolver == OrbitSolver::New &&
			std::abs( options.orbitRange - 2500.0f ) <= 0.000001f && options.maxFrames == 3780 &&
			options.qualityRung == QualityRung::HdrPost && options.motion == MotionMode::Static &&
			options.sceneFixture == SceneFixture::NewEden && options.composition == SceneComposition::Cinematic &&
			options.resolvedTaa == TaaMode::Off && options.resolvedDynamicExposure == DynamicExposure::Off &&
			options.resolvedBloom == PostFinishMode::Off && options.resolvedFilmGrain == PostFinishMode::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedVolumetrics == VolumetricMode::Off &&
			engineFixture && options.resolvedReflectionSource == ReflectionSource::Static &&
			options.resolvedAttachments == Attachments::Authored && options.resolvedDecals == Decals::Authored &&
			options.localLights == LocalLights::Authored && options.resolvedLocalShadows == LocalShadows::Off &&
			options.resolvedShadows == Shadows::Off && options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.planetLayers == PlanetLayers::All && options.resolvedSunEffects == SunEffects::Flare &&
			!options.ballparkLogPath.empty() && !options.capturePrefix.empty();
		if( !canonical )
		{
			std::cerr
				<< "--validate-ballpark-orbit requires the 3780-frame PL-11B Frontier-new 2500m hdr-post fixture\n";
			return false;
		}
	}
	if( options.validateBallparkWarp )
	{
		if( options.ballparkFrame == BallparkFrame::Chase )
		{
			std::cerr
				<< "The PL-11C quantitative contract accepts ego or fixed-observer frames; chase is visual-only\n";
			return false;
		}
		const bool engineFixture = options.ballparkFrame == BallparkFrame::Observer ?
			options.resolvedEngines == EngineMode::Authored :
			options.resolvedEngines == EngineMode::Off;
		const bool canonical = options.ballpark == BallparkMode::Warp && options.warpTarget == WarpTarget::Planet &&
			options.maxFrames == 3780 &&
			options.qualityRung == QualityRung::HdrPost && options.motion == MotionMode::Static &&
			options.sceneFixture == SceneFixture::NewEden && options.composition == SceneComposition::Cinematic &&
			options.resolvedTaa == TaaMode::Off && options.resolvedDynamicExposure == DynamicExposure::Off &&
			options.resolvedBloom == PostFinishMode::Off && options.resolvedFilmGrain == PostFinishMode::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedVolumetrics == VolumetricMode::Off &&
			engineFixture && options.resolvedReflectionSource == ReflectionSource::Static &&
			options.resolvedAttachments == Attachments::Authored && options.resolvedDecals == Decals::Authored &&
			options.localLights == LocalLights::Authored && options.resolvedLocalShadows == LocalShadows::Off &&
			options.resolvedShadows == Shadows::Off && options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.planetLayers == PlanetLayers::All && options.resolvedSunEffects == SunEffects::Flare &&
			!options.ballparkLogPath.empty() && !options.capturePrefix.empty();
		if( !canonical )
		{
			std::cerr << "--validate-ballpark-warp requires the 3780-frame PL-11C hdr-post warp fixture\n";
			return false;
		}
	}
	if( options.validateBallparkApproach )
	{
		if( options.ballparkFrame == BallparkFrame::Chase )
		{
			std::cerr
				<< "The PL-11D quantitative contract accepts ego or fixed-observer frames; chase is visual-only\n";
			return false;
		}
		const bool engineFixture = options.ballparkFrame == BallparkFrame::Observer ?
			options.resolvedEngines == EngineMode::Authored :
			options.resolvedEngines == EngineMode::Off;
		const bool canonical = options.ballpark == BallparkMode::Approach && options.maxFrames == 3780 &&
			options.qualityRung == QualityRung::HdrPost && options.motion == MotionMode::Static &&
			options.sceneFixture == SceneFixture::NewEden && options.composition == SceneComposition::Cinematic &&
			options.resolvedTaa == TaaMode::Off && options.resolvedDynamicExposure == DynamicExposure::Off &&
			options.resolvedBloom == PostFinishMode::Off && options.resolvedFilmGrain == PostFinishMode::Off &&
			options.resolvedDistortion == DistortionMode::Off && options.resolvedVolumetrics == VolumetricMode::Off &&
			engineFixture && options.resolvedReflectionSource == ReflectionSource::Static &&
			options.resolvedAttachments == Attachments::Authored && options.resolvedDecals == Decals::Authored &&
			options.localLights == LocalLights::Authored && options.resolvedLocalShadows == LocalShadows::Off &&
			options.resolvedShadows == Shadows::Off && options.resolvedAmbientOcclusion == AmbientOcclusion::Off &&
			options.planetLayers == PlanetLayers::All && options.resolvedSunEffects == SunEffects::Flare &&
			!options.ballparkLogPath.empty() && !options.capturePrefix.empty();
		if( !canonical )
		{
			std::cerr << "--validate-ballpark-approach requires the 3780-frame PL-11D hdr-post approach fixture\n";
			return false;
		}
	}
	if( options.celestialBallpark == CelestialBallparkMode::Natural &&
		( options.sceneFixture != SceneFixture::NewEden || options.composition != SceneComposition::System ||
		  options.ballpark == BallparkMode::Off ) )
	{
		std::cerr << "--celestial-ballpark natural requires the exact-system New Eden fixture and an active "
					 "Ballpark session\n";
		return false;
	}
	if( options.validateCelestialBallpark &&
		( options.celestialBallpark != CelestialBallparkMode::Natural || options.ballpark != BallparkMode::Orbit ||
		  options.orbitSolver != OrbitSolver::New || std::abs( options.orbitRange - 2500.0f ) > 0.000001f ||
		  options.maxFrames != 3780 || options.qualityRung != QualityRung::HdrPost ||
		  options.motion != MotionMode::Static ||
		  ( options.resolvedSunEffects != SunEffects::Flare && options.resolvedSunEffects != SunEffects::All ) ||
		  options.celestialLogPath.empty() || options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-celestial-ballpark requires the 3780-frame exact-system Frontier-new 2500m "
					 "hdr-post orbit fixture with natural celestials, sun flare effects, --celestial-log, and "
					 "--capture-prefix\n";
		return false;
	}
	if( options.eveGateTravel != 0 &&
		( options.eveGate != 1 || options.celestialBallpark != CelestialBallparkMode::Natural ||
		  options.ballpark != BallparkMode::Orbit || options.eveGateApproachFrame != 0 || options.validateBallpark ||
		  options.validateBallparkMotion || options.validateBallparkOrbit || options.validateBallparkWarp ||
		  options.validateBallparkApproach || options.validateCelestialBallpark || options.validateChaseCamera ||
		  options.validateEveGate ) )
	{
		std::cerr << "--eve-gate-travel direct is a demo mode requiring the authored gate, natural celestials, "
					 "and the ORBIT fixture; it replaces the orbit command and is incompatible with validation "
					 "fixtures and --eve-gate-approach\n";
		return false;
	}
	if( options.eveGateApproachFrame != 0 &&
		( options.ballpark != BallparkMode::Orbit || options.eveGateApproachFrame <= 180 || options.validateBallpark ||
		  options.validateBallparkMotion || options.validateBallparkOrbit || options.validateBallparkWarp ||
		  options.validateBallparkApproach || options.validateCelestialBallpark || options.validateChaseCamera ||
		  options.validateEveGate ) )
	{
		std::cerr << "--eve-gate-approach is a post-command ORBIT demo option and is incompatible with "
					 "Ballpark validation fixtures\n";
		return false;
	}
	if( options.eveGate != 0 &&
		( options.sceneFixture != SceneFixture::NewEden || options.composition != SceneComposition::System ) )
	{
		std::cerr << "--eve-gate authored requires the exact-system New Eden fixture\n";
		return false;
	}
	if( options.celestialAnchor != 0 &&
		( options.celestialBallpark != CelestialBallparkMode::Natural || options.validateBallpark ||
		  options.validateBallparkMotion || options.validateBallparkOrbit || options.validateBallparkWarp ||
		  options.validateBallparkApproach || options.validateCelestialBallpark || options.validateChaseCamera ||
		  options.validateEveGate ) )
	{
		std::cerr << "--celestial-anchor evegate is a demo anchor requiring natural celestials, and is "
					 "incompatible with validation fixtures\n";
		return false;
	}
	if( options.validateEveGate &&
		( options.eveGate != 1 || options.celestialBallpark != CelestialBallparkMode::Natural ||
		  options.ballpark != BallparkMode::Orbit || options.orbitSolver != OrbitSolver::New ||
		  std::abs( options.orbitRange - 2500.0f ) > 0.000001f || options.maxFrames != 3780 ||
		  options.qualityRung != QualityRung::HdrPost || options.motion != MotionMode::Static ||
		  options.capturePrefix.empty() ) )
	{
		std::cerr << "--validate-eve-gate requires the 3780-frame exact-system Frontier-new orbit fixture "
					 "with the authored gate, natural celestials, and --capture-prefix\n";
		return false;
	}
	if( options.validateChaseCamera &&
		( options.ballpark != BallparkMode::Goto || options.ballparkFrame != BallparkFrame::Chase ||
		  options.motion != MotionMode::Static || options.maxFrames != 1200 ||
		  options.qualityRung < QualityRung::Model || options.capturePrefix.empty() ||
		  options.ballparkLogPath.empty() ) )
	{
		std::cerr << "--validate-chase-camera requires a 1200-frame GOTO/chase Astero render with static sample "
					 "motion, --ballpark-log, and --capture-prefix\n";
		return false;
	}
	if( !options.captureFrames.empty() &&
		( options.captureEvery != 0 || options.captureStartFrame != 0 ||
		  options.capturePrefix.empty() || options.maxFrames <= options.captureFrames.back() ) )
	{
		std::cerr << "--capture-frames requires --capture-prefix, a --frames count beyond its largest frame, "
					 "and cannot be combined with --capture-every or --capture-start-frame\n";
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

bool CapturePixelBufferPng( const uint8_t* pixels,
							uint32_t width,
							uint32_t height,
							uint32_t pitch,
							const std::string& path )
{
	if( !pixels || !width || !height || !pitch || !EnsureParentDirectory( path ) )
	{
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

bool CaptureProbeProductPng( void* probe, const std::string& path )
{
	const uint8_t* pixels = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t pitch = 0;
	if( !TrinityStandaloneProbeGetCapturedProduct( probe, &pixels, &width, &height, &pitch ) )
	{
		std::cerr << "No synchronized render-product readback is available\n";
		return false;
	}
	return CapturePixelBufferPng( pixels, width, height, pitch, path );
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
	const std::string sunSuffix = "_sun-" + SunEffectsName( options.resolvedSunEffects ) + "_body-" +
		SunBodyLayersName( options.sunBodyLayers ) + "_att-" + AttachmentsName( options.resolvedAttachments ) + "-" +
		AttachmentViewName( options.attachmentView ) + "_decal-" + DecalsName( options.resolvedDecals ) + "-" +
		DecalViewName( options.decalView ) + "-kills-" + std::to_string( options.killCount ) + "_shadow-" +
		ShadowsName( options.resolvedShadows ) + "_ao-" + AmbientOcclusionName( options.resolvedAmbientOcclusion ) +
		( options.resolvedAmbientOcclusion == AmbientOcclusion::Off ? "" : "-" + AoMethodName( options.aoMethod ) );
	const std::string fullPath = options.capturePrefix + "_" + options.asset + "_" +
		QualityRungName( options.qualityRung ) + materialSuffix + sunSuffix + "_bloom-" +
		PostFinishModeName( options.resolvedBloom ) + "_grain-" + PostFinishModeName( options.resolvedFilmGrain ) +
		"_dist-" + DistortionModeName( options.resolvedDistortion ) + "_vol-" +
		VolumetricModeName( options.resolvedVolumetrics ) + "-" +
		VolumetricQualityName( options.resolvedVolumetricQuality ) + "_eng-" +
		EngineModeName( options.resolvedEngines ) + "-" + EngineViewName( options.engineView ) + "-thr-" +
		std::to_string( options.engineThrottle ) + "_bp-" + BallparkModeName( options.ballpark ) + "-" +
		BallparkFrameName( options.ballparkFrame );
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
	metadata << "solarIllumination=" << SolarIlluminationName( options.solarIllumination ) << "\n";
	metadata << "solarIlluminationView="
			 << SolarIlluminationViewName( options.solarIlluminationView ) << "\n";
	metadata << "captureFrames=";
	for( size_t index = 0; index < options.captureFrames.size(); ++index )
	{
		metadata << ( index ? "," : "" ) << options.captureFrames[index];
	}
	metadata << "\n";
	metadata << "aoRequested=" << AmbientOcclusionName( options.ambientOcclusion ) << "\n";
	metadata << "aoResolved=" << AmbientOcclusionName( options.resolvedAmbientOcclusion ) << "\n";
	metadata << "aoMethod=" << AoMethodName( options.aoMethod ) << "\n";
	metadata << "renderProduct=" << RenderProductName( options.renderProduct ) << "\n";
	metadata << "taaRequested=" << TaaModeName( options.taa ) << "\n";
	metadata << "taaResolved=" << TaaModeName( options.resolvedTaa ) << "\n";
	metadata << "taaDebug=" << TaaDebugName( options.taaDebug ) << "\n";
	metadata << "motion=" << MotionModeName( options.motion ) << "\n";
	metadata << "ballpark=" << BallparkModeName( options.ballpark ) << "\n";
	metadata << "warpTarget=" << ( options.warpTarget == WarpTarget::EveGate ? "evegate" : "planet" ) << "\n";
	metadata << "ballparkFrame=" << BallparkFrameName( options.ballparkFrame ) << "\n";
	metadata << "validateBallpark=" << ( options.validateBallpark ? "true" : "false" ) << "\n";
	metadata << "validateBallparkMotion=" << ( options.validateBallparkMotion ? "true" : "false" ) << "\n";
	metadata << "validateChaseCamera=" << ( options.validateChaseCamera ? "true" : "false" ) << "\n";
	metadata << "ballparkLog=" << options.ballparkLogPath << "\n";
	metadata << "celestialBallpark="
			 << ( options.celestialBallpark == CelestialBallparkMode::Natural ? "natural" : "off" ) << "\n";
	metadata << "validateCelestialBallpark=" << ( options.validateCelestialBallpark ? "true" : "false" ) << "\n";
	metadata << "celestialLog=" << options.celestialLogPath << "\n";
	metadata << "temporalValidation=" << ( options.validateTemporal ? "requested" : "off" ) << "\n";
	metadata << "temporalTest=" << TemporalTestName( options.temporalTest ) << "\n";
	metadata << "cameraView=" << CameraViewName( options.cameraView ) << "\n";
	metadata << "composition=" << SceneCompositionName( options.composition ) << "\n";
	metadata << "planetLayers=" << PlanetLayersName( options.planetLayers ) << "\n";
	metadata << "planetCloudDate=" << PlanetCloudDateName( options ) << "\n";
	metadata << "sunEffectsRequested=" << SunEffectsName( options.sunEffects ) << "\n";
	metadata << "sunEffectsResolved=" << SunEffectsName( options.resolvedSunEffects ) << "\n";
	metadata << "sunBodyLayers=" << SunBodyLayersName( options.sunBodyLayers ) << "\n";
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
	metadata << "volumetricsRequested=" << VolumetricModeName( options.volumetrics ) << "\n";
	metadata << "volumetricsResolved=" << VolumetricModeName( options.resolvedVolumetrics ) << "\n";
	metadata << "volumetricQualityRequested=" << VolumetricQualityName( options.volumetricQuality ) << "\n";
	metadata << "volumetricQualityResolved=" << VolumetricQualityName( options.resolvedVolumetricQuality ) << "\n";
	metadata << "volumetricSeed=" << options.volumetricSeed << "\n";
	metadata << "froxelTemporal=" << ( options.froxelTemporal ? "on" : "off" ) << "\n";
	metadata << "volumetricClientPolicy=high; ultra-diagnostic-only\n";
	metadata << "volumetricFixtureAuthorship=silk-client-authored; froxel-sample-owned\n";
	metadata
		<< "volumetricFixturePlacement=sample-authored radius=2.2*modelWorldScale position=(-3.2,1.8,3.8)*modelWorldScale\n";
	metadata << "validateVolumetrics=" << ( options.validateVolumetrics ? "true" : "false" ) << "\n";
	metadata << "validateDistortion=" << ( options.validateDistortion ? "true" : "false" ) << "\n";
	metadata << "enginesRequested=" << EngineModeName( options.engines ) << "\n";
	metadata << "enginesResolved=" << EngineModeName( options.resolvedEngines ) << "\n";
	metadata << "engineView=" << EngineViewName( options.engineView ) << "\n";
	metadata << "engineThrottle=" << options.engineThrottle << "\n";
	metadata << "engineKinematics=sample-owned-camera-follow-cruise-max-velocity-250mps-zero-acceleration\n";
	metadata << "engineParticles=none-client-builder-has-no-separate-astero-particle-branch\n";
	metadata << "validateEngines=" << ( options.validateEngines ? "true" : "false" ) << "\n";
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
	const double compensation = std::clamp( 0.5 / luminance,
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

bool EvaluateExposureSamples( const Options& options,
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
		const uint64_t histogramTotal =
			std::accumulate( std::begin( sample.histogram ), std::begin( sample.histogram ) + 64, uint64_t{ 0 } );
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
		const double expectedRoot =
			std::sqrt( previous.exposure[0] ) + ( std::sqrt( target ) - std::sqrt( previous.exposure[0] ) ) * alpha;
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
				( std::sqrt( current.exposure[0] ) - std::sqrt( previous.exposure[0] ) ) / denominator, 0.0, 0.999999 );
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
		for( size_t index = std::min( samples.size(), hold * 2 ) - window; index < std::min( samples.size(), hold * 2 );
			 ++index )
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
	out << std::fixed << std::setprecision( 8 ) << "samples=" << samples.size() << " invalidFrames=" << invalidFrames
		<< " histogramMismatches=" << histogramMismatches << " recurrenceMismatches=" << recurrenceMismatches
		<< " maximumRecurrenceError=" << maximumRecurrenceError << " overshoots=" << overshoots
		<< " increaseRate=" << Median( increaseRates ) << " decreaseRate=" << Median( decreaseRates )
		<< " stepFraction=" << stepFraction << " fittedSequenceRate=" << fittedRate
		<< " terminalTrackingError=" << terminalTrackingError << " validation=" << ( passed ? "pass" : "fail" );
	summary = out.str();
	std::cerr << "EVE RC-10 exposure validation: " << summary << "\n";
	return passed;
}

bool WriteExposureCsv( const std::string& path, const std::vector<TrinityStandalonePostProcessDiagnostics>& samples )
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
	output
		<< "frame,realTime,simTime,adaptedLuminance,sceneTime,reserved2,lowerBin,upperBin,lowerLuminance,upperLuminance,reserved7";
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

bool WriteToneContractJson( const Options& options,
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
		   << "  \"preTonemapHash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << tone.preTonemapHash
		   << "\",\n"
		   << "  \"postTonemapHash\": \"" << std::setw( 16 ) << tone.postTonemapHash << std::dec << "\",\n"
		   << "  \"dimensions\": [" << tone.width << ", " << tone.height << "],\n"
		   << "  \"fullyBlackPixels\": " << tone.fullyBlackPixels << ",\n"
		   << "  \"fullyWhitePixels\": " << tone.fullyWhitePixels << ",\n"
		   << "  \"anySaturatedChannelPixels\": " << tone.anySaturatedChannelPixels << ",\n"
		   << "  \"luminance\": {\"p01\": " << tone.luminanceP01 << ", \"p50\": " << tone.luminanceP50
		   << ", \"p99\": " << tone.luminanceP99 << "},\n"
		   << "  \"cpuReferenceError\": {\"mean\": " << tone.meanAbsoluteError
		   << ", \"p999\": " << tone.p999AbsoluteError << ", \"maximum\": " << tone.maximumAbsoluteError << "},\n"
		   << "  \"exposureValidation\": \"" << exposureSummary << "\",\n"
		   << "  \"validationPassed\": " << ( tone.valid ? "true" : "false" ) << ",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Tone contract report: " << outputPath << "\n";
	return output.good();
}

bool CompareToneValidations( const TrinityStandaloneToneValidation& off,
							 const TrinityStandaloneToneValidation& client,
							 std::string& summary )
{
	const uint64_t pixelCount = static_cast<uint64_t>( client.width ) * client.height;
	const bool sameInput = off.preTonemapHash == client.preTonemapHash;
	const bool distinctOutput = off.postTonemapHash != client.postTonemapHash;
	const bool reducedWhite = client.fullyWhitePixels < off.fullyWhitePixels;
	const bool clippingValid = client.fullyWhitePixels <= pixelCount / 1000 &&
		client.anySaturatedChannelPixels <= pixelCount * 3 / 100 && client.fullyBlackPixels <= pixelCount / 1000;
	const bool lowEndRetained = client.luminanceP01 >= off.luminanceP01 * 0.75;
	const bool passed =
		off.valid && client.valid && sameInput && distinctOutput && reducedWhite && clippingValid && lowEndRetained;
	std::ostringstream out;
	out << "sameInput=" << ( sameInput ? "true" : "false" )
		<< " distinctOutput=" << ( distinctOutput ? "true" : "false" ) << " offWhite=" << off.fullyWhitePixels
		<< " clientWhite=" << client.fullyWhitePixels << " clientAnySaturated=" << client.anySaturatedChannelPixels
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

bool WriteDistortionContractJson( const Options& options, const TrinityStandaloneDistortionDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_distortion-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/AsteroDistortionResources.json";
	const std::string generatedCmfHashPath = ExecutableDirectory() + "/../Reports/AsteroDistortionGeneratedCmf.sha256";
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
		   << "  \"mapHash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << diagnostics.mapHash
		   << "\",\n"
		   << "  \"offPreTonemapHash\": \"" << std::setw( 16 ) << diagnostics.offPreTonemapHash << "\",\n"
		   << "  \"authoredPreTonemapHash\": \"" << std::setw( 16 ) << diagnostics.authoredPreTonemapHash << "\",\n"
		   << "  \"offFinalHash\": \"" << std::setw( 16 ) << diagnostics.offFinalHash << "\",\n"
		   << "  \"authoredFinalHash\": \"" << std::setw( 16 ) << diagnostics.authoredFinalHash << std::dec << "\",\n"
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
		   << "  \"sceneColorCopySucceeded\": " << ( diagnostics.copySucceeded ? "true" : "false" ) << ",\n"
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

bool WriteVolumetricContractJson( const Options& options, const TrinityStandaloneVolumetricDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_volumetric-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/VolumetricResources.json";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	if( !manifestInput || manifest.str().empty() )
	{
		std::cerr << "RC-12B1 could not read the volumetric resource manifest: " << manifestPath << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	output << "{\n"
		   << "  \"profile\": \"RC-12B1\",\n"
		   << "  \"fixture\": \"res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_graybrown.black\",\n"
		   << "  \"authorship\": \"client-authored cloud, sample-authored placement\",\n"
		   << "  \"placement\": {\"targetRadiusScale\": 2.2, \"positionScale\": [-3.2, 1.8, 3.8]},\n"
		   << "  \"quality\": \"" << VolumetricQualityName( options.resolvedVolumetricQuality ) << "\",\n"
		   << "  \"resolutionScale\": 0.7,\n"
		   << "  \"seed\": " << diagnostics.seed << ",\n"
		   << "  \"volume\": {\"dimensions\": [" << diagnostics.volumeWidth << ", " << diagnostics.volumeHeight << ", "
		   << diagnostics.volumeLayers << "], \"format\": " << diagnostics.volumeFormat
		   << ", \"copySucceeded\": " << ( diagnostics.localOutputCopySucceeded ? "true" : "false" ) << "},\n"
		   << "  \"passes\": {\"renderables\": " << diagnostics.localRenderableCount
		   << ", \"batches\": " << diagnostics.localBatchCount
		   << ", \"depthDownsample\": " << ( diagnostics.localDepthDownsampleSucceeded ? "true" : "false" )
		   << ", \"blur\": " << ( diagnostics.localBlurSucceeded ? "true" : "false" )
		   << ", \"composition\": " << ( diagnostics.localBlitSucceeded ? "true" : "false" ) << "},\n"
		   << "  \"lightmap\": {\"updates\": " << diagnostics.totalLocalLightmapUpdates
		   << ", \"settled\": " << ( diagnostics.localLightmapSettled ? "true" : "false" ) << "},\n"
		   << "  \"cloudShadows\": {\"lastBatches\": " << diagnostics.localShadowBatchCount
		   << ", \"totalBatches\": " << diagnostics.totalLocalShadowBatches << "},\n"
		   << "  \"visualization\": {\"hash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' )
		   << diagnostics.volumeProductHash << std::dec
		   << "\", \"nonzeroPixels\": " << diagnostics.volumeProductNonzeroPixels << ", \"range\": ["
		   << static_cast<unsigned>( diagnostics.volumeProductMinimum ) << ", "
		   << static_cast<unsigned>( diagnostics.volumeProductMaximum ) << "], \"bounds\": ["
		   << diagnostics.volumeProductMinX << ", " << diagnostics.volumeProductMinY << ", "
		   << diagnostics.volumeProductMaxX << ", " << diagnostics.volumeProductMaxY << "]},\n"
		   << "  \"rawVolume\": {\"hash\": \"" << std::hex << std::setw( 16 ) << std::setfill( '0' )
		   << diagnostics.volumeRawHash << std::dec << "\", \"nonzeroPixels\": " << diagnostics.volumeRawNonzeroPixels
		   << ", \"range\": [" << diagnostics.volumeRawMinimum << ", " << diagnostics.volumeRawMaximum << "]},\n"
		   << "  \"matchedHashes\": {\n"
		   << std::hex << std::setfill( '0' ) << "    \"preTonemap\": {\"off\": \"" << std::setw( 16 )
		   << diagnostics.offPreTonemapHash << "\", \"silk\": \"" << std::setw( 16 ) << diagnostics.silkPreTonemapHash
		   << "\"},\n"
		   << "    \"final\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offFinalHash << "\", \"silk\": \""
		   << std::setw( 16 ) << diagnostics.silkFinalHash << "\"},\n"
		   << "    \"shadow\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offShadowHash << "\", \"silk\": \""
		   << std::setw( 16 ) << diagnostics.silkShadowHash << "\"},\n"
		   << "    \"depth\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offDepthHash << "\", \"silk\": \""
		   << std::setw( 16 ) << diagnostics.silkDepthHash << "\"},\n"
		   << "    \"normal\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offNormalHash << "\", \"silk\": \""
		   << std::setw( 16 ) << diagnostics.silkNormalHash << "\"},\n"
		   << "    \"cascadeAtlas\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offShadowAtlasHash
		   << "\", \"silk\": \"" << std::setw( 16 ) << diagnostics.silkShadowAtlasHash << "\"},\n"
		   << "    \"ao\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offAoHash << "\", \"silk\": \""
		   << std::setw( 16 ) << diagnostics.silkAoHash << "\"},\n"
		   << "    \"bentNormal\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offBentNormalHash
		   << "\", \"silk\": \"" << std::setw( 16 ) << diagnostics.silkBentNormalHash << "\"}\n"
		   << std::dec << "  },\n"
		   << "  \"validationPassed\": " << ( diagnostics.valid ? "true" : "false" ) << ",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Volumetric contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteEngineContractJson( const Options& options, const TrinityStandaloneEngineDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_engine-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/AsteroEngineResources.json";
	const std::string generatedCmfHashPath = ExecutableDirectory() + "/../Reports/AsteroEngineTrailCmf.sha256";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	std::ifstream generatedCmfHashInput( generatedCmfHashPath );
	std::string generatedCmfHash;
	std::getline( generatedCmfHashInput, generatedCmfHash );
	if( !manifestInput || manifest.str().empty() || !generatedCmfHashInput || generatedCmfHash.empty() )
	{
		std::cerr << "RC-05D could not read engine provenance reports: " << manifestPath << ", " << generatedCmfHashPath
				  << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	output << "{\n"
		   << "  \"profile\": \"RC-05D\",\n"
		   << "  \"requestedMode\": \"" << EngineModeName( options.engines ) << "\",\n"
		   << "  \"resolvedMode\": \"" << EngineModeName( options.resolvedEngines ) << "\",\n"
		   << "  \"view\": \"" << EngineViewName( options.engineView ) << "\",\n"
		   << "  \"kinematics\": {\"authorship\": \"sample-owned camera-follow cruise\", "
			  "\"maximumVelocity\": 250, \"throttle\": "
		   << diagnostics.throttle << ", \"nativeIntensity\": " << diagnostics.nativeIntensity
		   << ", \"normalizedIntensity\": " << diagnostics.intensity
		   << ", \"nativeCruiseSpeedWeight\": 0.8, \"acceleration\": [0, 0, 0]},\n"
		   << "  \"inventory\": {\"boosters\": " << diagnostics.boosterCount
		   << ", \"plumeBatches\": " << diagnostics.plumeBatchCount
		   << ", \"plumeInstances\": " << diagnostics.plumeInstanceCount << ", \"glows\": " << diagnostics.glowCount
		   << ", \"glowSubmissions\": " << diagnostics.glowSubmissionCount << ", \"trails\": " << diagnostics.trailCount
		   << ", \"trailPrimitives\": " << diagnostics.trailPrimitiveCount
		   << ", \"trailBatches\": " << diagnostics.trailBatchCount
		   << ", \"trailInstances\": " << diagnostics.trailInstanceCount
		   << ", \"lights\": " << diagnostics.lightSubmissionCount << "},\n"
		   << "  \"trail\": {\"length\": " << diagnostics.trailLength
		   << ", \"intensity\": " << diagnostics.trailIntensity << "},\n"
		   << "  \"separateParticleBranch\": false,\n"
		   << "  \"matchedHashes\": {\n"
		   << std::hex << std::setfill( '0' ) << "    \"preTonemap\": {\"off\": \"" << std::setw( 16 )
		   << diagnostics.offPreTonemapHash << "\", \"authored\": \"" << std::setw( 16 )
		   << diagnostics.authoredPreTonemapHash << "\"},\n"
		   << "    \"bloom\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offBloomHash << "\", \"authored\": \""
		   << std::setw( 16 ) << diagnostics.authoredBloomHash << "\"},\n"
		   << "    \"final\": {\"off\": \"" << std::setw( 16 ) << diagnostics.offFinalHash << "\", \"authored\": \""
		   << std::setw( 16 ) << diagnostics.authoredFinalHash << "\"}\n"
		   << std::dec << "  },\n"
		   << "  \"ready\": " << ( diagnostics.ready ? "true" : "false" ) << ",\n"
		   << "  \"validationPassed\": " << ( diagnostics.valid ? "true" : "false" ) << ",\n"
		   << "  \"generatedTrailCmfHashPath\": \"" << generatedCmfHashPath << "\",\n"
		   << "  \"generatedTrailCmfHash\": \"" << generatedCmfHash << "\",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Engine contract report: " << outputPath << "\n";
	return output.good();
}

bool EvaluateTemporalSamples( const Options& options,
							  const std::vector<TrinityStandalonePostProcessDiagnostics>& samples,
							  uint32_t width,
							  uint32_t height,
							  std::string& summary )
{
	if( samples.size() < 4 )
	{
		summary = "validation=fail reason=insufficient-samples";
		return false;
	}

	bool valid = true;
	uint32_t resetCount = 0;
	std::array<std::pair<float, float>, 4> phases = {};
	const size_t phaseStart = samples.size() - phases.size();
	for( size_t index = 0; index < samples.size(); ++index )
	{
		const auto& sample = samples[index];
		resetCount += sample.taaReset ? 1u : 0u;
		valid = valid && sample.taaActive && sample.taaAccumulationSucceeded && sample.taaCopySucceeded &&
			sample.taaQuality == 3 && sample.velocityWidth == width && sample.velocityHeight == height &&
			sample.velocityFormat != 0 && sample.opaqueWidth == width && sample.opaqueHeight == height &&
			sample.opaqueFormat != 0 && sample.taaAccumulationWidth == width &&
			sample.taaAccumulationHeight == height && sample.taaAccumulationFormat != 0 &&
			sample.taaCooldownWidth == width && sample.taaCooldownHeight == height && sample.taaCooldownFormat != 0 &&
			std::isfinite( sample.taaBlendWeight ) && sample.taaBlendWeight >= 0.0f &&
			sample.taaBlendWeight <= 0.960001f && std::abs( sample.taaEarlyOutThreshold - 0.001f ) <= 0.000001f;
		if( index >= phaseStart )
		{
			phases[index - phaseStart] = { sample.taaJitterPixelX, sample.taaJitterPixelY };
		}
	}
	for( size_t left = 0; left < phases.size(); ++left )
	{
		for( size_t right = left + 1; right < phases.size(); ++right )
		{
			valid = valid &&
				( std::abs( phases[left].first - phases[right].first ) > 0.0001f ||
				  std::abs( phases[left].second - phases[right].second ) > 0.0001f );
		}
	}
	const auto& final = samples.back();
	std::ostringstream stats;
	stats << "validation=" << ( valid ? "pass" : "fail" ) << " samples=" << samples.size() << " resets=" << resetCount
		  << " frameIndex=" << final.taaFrameIndex << " quality=" << final.taaQuality
		  << " velocity=" << final.velocityWidth << "x" << final.velocityHeight << "/" << final.velocityFormat
		  << " accumulation=" << final.taaAccumulationWidth << "x" << final.taaAccumulationHeight << "/"
		  << final.taaAccumulationFormat << " cooldown=" << final.taaCooldownWidth << "x" << final.taaCooldownHeight
		  << "/" << final.taaCooldownFormat << " blend=" << final.taaBlendWeight
		  << " threshold=" << final.taaEarlyOutThreshold << " motion=" << MotionModeName( options.motion )
		  << " velocityHash=" << std::hex << final.velocityRawHash << std::dec
		  << " velocityMoving=" << final.velocityMovingPixels << " velocityMean=" << final.velocityMeanMagnitude
		  << " velocityMax=" << final.velocityMaximumMagnitude << " cooldownHash=" << std::hex
		  << final.taaCooldownRawHash << std::dec << " cooldownNonzero=" << final.taaCooldownNonzeroPixels
		  << " cooldownRange=" << final.taaCooldownMinimum << "-" << final.taaCooldownMaximum;
	summary = stats.str();
	return valid;
}

bool WriteTemporalContractJson( const Options& options,
								const std::vector<TrinityStandalonePostProcessDiagnostics>& samples,
								const TrinityStandaloneTemporalValidation& validation,
								const std::string& validationSummary,
								bool validationPassed )
{
	const std::string outputPath = CaptureBasePath( options ) + "_temporal-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
	{
		return false;
	}
	const std::string manifestPath = ExecutableDirectory() + "/../Reports/AsteroTemporalResources.json";
	const std::string codeHashPath = ExecutableDirectory() + "/../Reports/EveCodeCcp.sha256";
	const char* home = std::getenv( "HOME" );
	const std::string codePath = std::string( home ? home : "" ) +
		"/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/code.ccp";
	std::ifstream manifestInput( manifestPath );
	std::ostringstream manifest;
	manifest << manifestInput.rdbuf();
	std::ifstream codeHashInput( codeHashPath );
	std::string codeHashLine;
	std::getline( codeHashInput, codeHashLine );
	std::istringstream codeHashStream( codeHashLine );
	std::string codeHash;
	codeHashStream >> codeHash;
	if( !manifestInput || manifest.str().empty() || !codeHashInput || codeHash.empty() )
	{
		std::cerr << "RC-13 could not read temporal provenance reports: " << manifestPath << ", " << codeHashPath
				  << "\n";
		return false;
	}
	std::ofstream output( outputPath );
	if( !output )
	{
		return false;
	}
	const TrinityStandalonePostProcessDiagnostics emptyDiagnostics = {};
	const auto& final = samples.empty() ? emptyDiagnostics : samples.back();
	output << "{\n"
		   << "  \"profile\": \"RC-13\",\n"
		   << "  \"test\": \"" << TemporalTestName( options.temporalTest ) << "\",\n"
		   << "  \"requestedTaa\": \"" << TaaModeName( options.taa ) << "\",\n"
		   << "  \"resolvedTaa\": \"" << TaaModeName( options.resolvedTaa ) << "\",\n"
		   << "  \"debugMode\": \"" << TaaDebugName( options.taaDebug ) << "\",\n"
		   << "  \"motion\": \"" << MotionModeName( options.motion ) << "\",\n"
		   << "  \"motionSchedule\": \"frames 0-179 static; camera/object motion begins at frame 180\",\n"
		   << "  \"samples\": " << samples.size() << ",\n"
		   << "  \"frameIndex\": " << final.taaFrameIndex << ",\n"
		   << "  \"quality\": " << final.taaQuality << ",\n"
		   << "  \"blendWeight\": " << final.taaBlendWeight << ",\n"
		   << "  \"earlyOutThreshold\": " << final.taaEarlyOutThreshold << ",\n"
		   << "  \"jitterPixels\": [" << final.taaJitterPixelX << ", " << final.taaJitterPixelY << "],\n"
		   << "  \"velocity\": {\"width\": " << final.velocityWidth << ", \"height\": " << final.velocityHeight
		   << ", \"format\": " << final.velocityFormat << ", \"rawHash\": \"" << std::hex << final.velocityRawHash
		   << std::dec << "\", \"finitePixels\": " << final.velocityFinitePixels
		   << ", \"invalidPixels\": " << final.velocityInvalidPixels
		   << ", \"movingPixels\": " << final.velocityMovingPixels
		   << ", \"meanMagnitude\": " << final.velocityMeanMagnitude
		   << ", \"maximumMagnitude\": " << final.velocityMaximumMagnitude << "},\n"
		   << "  \"accumulation\": {\"width\": " << final.taaAccumulationWidth
		   << ", \"height\": " << final.taaAccumulationHeight << ", \"format\": " << final.taaAccumulationFormat
		   << "},\n"
		   << "  \"cooldown\": {\"width\": " << final.taaCooldownWidth << ", \"height\": " << final.taaCooldownHeight
		   << ", \"format\": " << final.taaCooldownFormat << "},\n"
		   << "  \"cooldownReadback\": {\"rawHash\": \"" << std::hex << final.taaCooldownRawHash << std::dec
		   << "\", \"nonzeroPixels\": " << final.taaCooldownNonzeroPixels
		   << ", \"minimum\": " << final.taaCooldownMinimum << ", \"maximum\": " << final.taaCooldownMaximum << "},\n"
		   << "  \"atomicCapture\": {\n"
		   << "    \"samples\": " << validation.sampleCount << ", \"width\": " << validation.width
		   << ", \"height\": " << validation.height << ",\n"
		   << "    \"formats\": {\"depth\": " << validation.depthFormat
		   << ", \"velocity\": " << validation.velocityFormat << ", \"opaque\": " << validation.opaqueFormat
		   << ", \"preTaa\": " << validation.preTaaFormat << ", \"postTaa\": " << validation.postTaaFormat
		   << ", \"cooldown\": " << validation.cooldownFormat << "},\n"
		   << "    \"hashes\": {\"depth\": \"" << std::hex << validation.depthHash << "\", \"velocity\": \""
		   << validation.velocityHash << "\", \"opaque\": \"" << validation.opaqueHash << "\", \"preTaa\": \""
		   << validation.preTaaHash << "\", \"postTaa\": \"" << validation.postTaaHash << "\", \"cooldown\": \""
		   << validation.cooldownHash << std::dec << "\"},\n"
		   << "    \"velocity\": {\"interiorSamples\": " << validation.velocityInteriorSamples
		   << ", \"excludedDiscontinuities\": " << validation.velocityExcludedDiscontinuities
		   << ", \"invalidSamples\": " << validation.velocityInvalidSamples
		   << ", \"staticMaximum\": " << validation.velocityStaticMaximum
		   << ", \"meanError\": " << validation.velocityMeanError << ", \"p99Error\": " << validation.velocityP99Error
		   << ", \"maximumError\": " << validation.velocityMaximumError << "},\n"
		   << "    \"edges\": {\"pixels\": " << validation.edgePixels
		   << ", \"preVarianceP95\": " << validation.preTaaEdgeVarianceP95
		   << ", \"postVarianceP95\": " << validation.postTaaEdgeVarianceP95
		   << ", \"ratio\": " << validation.edgeVarianceRatio
		   << ", \"preMaximumLuminance\": " << validation.preTaaMaximumLuminance
		   << ", \"postMaximumLuminance\": " << validation.postTaaMaximumLuminance << "},\n"
		   << "    \"transients\": {\"currentPixels\": " << validation.currentTransientPixels
		   << ", \"previousOnlyPixels\": " << validation.previousOnlyTransientPixels
		   << ", \"cooldownActivePixels\": " << validation.cooldownActivePixels
		   << ", \"cooldownMaximum\": " << validation.cooldownMaximum
		   << ", \"cooldownOverlapPixels\": " << validation.cooldownOverlapPixels
		   << ", \"cooldownOverlapFraction\": " << validation.cooldownOverlapFraction
		   << ", \"currentEnergy\": " << validation.currentTransientEnergy
		   << ", \"previousOnlyResidualEnergy\": " << validation.previousOnlyResidualEnergy
		   << ", \"residualRatio\": " << validation.transientResidualRatio << "}\n"
		   << "  },\n"
		   << "  \"validationSummary\": \"" << validationSummary << "\",\n"
		   << "  \"validationPassed\": " << ( validationPassed ? "true" : "false" ) << ",\n"
		   << "  \"codeCcpPath\": \"" << codePath << "\",\n"
		   << "  \"codeCcpSha256\": \"" << codeHash << "\",\n"
		   << "  \"resourceManifestPath\": \"" << manifestPath << "\",\n"
		   << "  \"resourceManifest\": " << manifest.str() << "\n"
		   << "}\n";
	std::cout << "Temporal contract report: " << outputPath << "\n";
	return output.good();
}

bool CaptureBallparkValidationPngs( void* probe, const Options& options, bool& colorEqual, bool& depthEqual )
{
	const std::string base = CaptureBasePath( options );
	std::array<std::string, 4> paths = {
		base + "_ballpark-off-color.png",
		base + "_ballpark-off-depth.png",
		base + "_ballpark-static-color.png",
		base + "_ballpark-static-depth.png",
	};
	for( int staticMode = 0; staticMode < 2; ++staticMode )
	{
		for( int depthProduct = 0; depthProduct < 2; ++depthProduct )
		{
			const uint8_t* pixels = nullptr;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t pitch = 0;
			if( !TrinityStandaloneProbeGetBallparkCapture(
					probe, staticMode, depthProduct, &pixels, &width, &height, &pitch ) ||
				!CapturePixelBufferPng( pixels, width, height, pitch, paths[staticMode * 2 + depthProduct] ) )
			{
				return false;
			}
		}
	}
	auto equalFiles = []( const std::string& left, const std::string& right ) {
		NSData* leftData = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:left.c_str()]];
		NSData* rightData = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:right.c_str()]];
		return leftData && rightData && [leftData isEqualToData:rightData];
	};
	colorEqual = equalFiles( paths[0], paths[2] );
	depthEqual = equalFiles( paths[1], paths[3] );
	return colorEqual && depthEqual;
}

bool WriteBallparkContractJson( const Options& options,
								const TrinityStandaloneBallparkDiagnostics& diagnostics,
								bool encodedColorEqual,
								bool encodedDepthEqual )
{
	const std::string outputPath = CaptureBasePath( options ) + "_ballpark-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-10 first Ballpark\",\n"
		   << "  \"mode\": \"" << BallparkModeName( options.ballpark ) << "\",\n"
		   << "  \"csv\": \"" << options.ballparkLogPath << "\",\n"
		   << "  \"pythonPolicy\": \"initialized-required; no Destiny module\",\n"
		   << "  \"pythonInitialized\": " << ( diagnostics.pythonInitialized ? "true" : "false" ) << ",\n"
		   << "  \"destinyPythonModulesAbsent\": " << ( diagnostics.destinyPythonModulesAbsent ? "true" : "false" )
		   << ",\n"
		   << "  \"registeredClasses\": " << diagnostics.registeredClassCount << ",\n"
		   << "  \"loadedBlueImages\": " << diagnostics.loadedBlueImageCount << ",\n"
		   << "  \"loadedPythonImages\": " << diagnostics.loadedPythonImageCount << ",\n"
		   << "  \"directEvolves\": " << diagnostics.directEvolveCount << ",\n"
		   << "  \"originUpdates\": " << diagnostics.originUpdateCount << ",\n"
		   << "  \"schedulerRegistered\": " << ( diagnostics.schedulerRegistered ? "true" : "false" ) << ",\n"
		   << "  \"startCalls\": " << diagnostics.startCallCount << ",\n"
		   << "  \"onTickCalls\": " << diagnostics.onTickCallCount << ",\n"
		   << "  \"pythonCallbacks\": " << diagnostics.pythonCallbackCount << ",\n"
		   << "  \"unitBase\": " << diagnostics.unitBase << ",\n"
		   << "  \"hashes\": {\"offColor\": \"" << std::hex << diagnostics.offColorHash << "\", \"staticColor\": \""
		   << diagnostics.staticColorHash << "\", \"offDepth\": \"" << diagnostics.offDepthHash
		   << "\", \"staticDepth\": \"" << diagnostics.staticDepthHash << "\"},\n"
		   << std::dec << "  \"worldMatricesEqual\": " << ( diagnostics.worldMatricesEqual ? "true" : "false" ) << ",\n"
		   << "  \"colorHashesEqual\": " << ( diagnostics.colorHashesEqual ? "true" : "false" ) << ",\n"
		   << "  \"depthHashesEqual\": " << ( diagnostics.depthHashesEqual ? "true" : "false" ) << ",\n";
	output << "  \"encodedColorCapturesEqual\": " << ( encodedColorEqual ? "true" : "false" ) << ",\n"
		   << "  \"encodedDepthCapturesEqual\": " << ( encodedDepthEqual ? "true" : "false" ) << ",\n";
	writeArray( "position", diagnostics.position, 3, true );
	writeArray( "rotation", diagnostics.rotation, 4, true );
	writeArray( "referencePoint", diagnostics.referencePoint, 3, true );
	writeArray( "origin", diagnostics.origin, 3, true );
	writeArray( "originShift", diagnostics.originShift, 3, true );
	writeArray( "delta", diagnostics.delta, 3, true );
	writeArray( "smoothedDelta", diagnostics.smoothedDelta, 3, true );
	writeArray( "deltaVelocity", diagnostics.deltaVelocity, 3, true );
	writeArray( "offWorld", diagnostics.offWorld, 16, true );
	writeArray( "staticWorld", diagnostics.staticWorld, 16, true );
	output << "  \"validationPassed\": "
		   << ( diagnostics.valid && encodedColorEqual && encodedDepthEqual ? "true" : "false" ) << "\n}\n";
	std::cout << "Ballpark contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteBallparkMotionContractJson( const Options& options,
									  const TrinityStandaloneBallparkDiagnostics& diagnostics,
									  bool milestonesCaptured )
{
	const std::string outputPath = CaptureBasePath( options ) + "_ballpark-motion-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-11A native STOP and linear GOTO\",\n"
		   << "  \"mode\": \"" << BallparkModeName( options.ballpark ) << "\",\n"
		   << "  \"referenceFrame\": \"" << BallparkFrameName( options.ballparkFrame ) << "\",\n"
		   << "  \"csv\": \"" << options.ballparkLogPath << "\",\n"
		   << "  \"primaryBallId\": " << diagnostics.primaryBallId << ",\n"
		   << "  \"egoBallId\": " << diagnostics.egoBallId << ",\n"
		   << "  \"directEvolves\": " << diagnostics.directEvolveCount << ",\n"
		   << "  \"commands\": " << diagnostics.commandCount << ",\n"
		   << "  \"lastCommandTime\": " << diagnostics.lastCommandTime << ",\n"
		   << "  \"orientationPins\": " << diagnostics.orientationPinCount << ",\n"
		   << "  \"originUpdates\": " << diagnostics.originUpdateCount << ",\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"maximumErrors\": {\"position\": " << diagnostics.maximumRawPositionError
		   << ", \"velocity\": " << diagnostics.maximumRawVelocityError
		   << ", \"acceleration\": " << diagnostics.maximumRawAccelerationError
		   << ", \"curve\": " << diagnostics.maximumCurveError << ", \"root\": " << diagnostics.maximumRootError
		   << ", \"origin\": " << diagnostics.maximumOriginError << "},\n"
		   << "  \"initialRoll\": " << diagnostics.initialRoll << ",\n"
		   << "  \"finalRoll\": " << diagnostics.finalRoll << ",\n"
		   << "  \"overshootObserved\": " << ( diagnostics.overshootObserved ? "true" : "false" ) << ",\n"
		   << "  \"reversalObserved\": " << ( diagnostics.reversalObserved ? "true" : "false" ) << ",\n"
		   << "  \"nativeOrientationChanged\": " << ( diagnostics.nativeOrientationChanged ? "true" : "false" ) << ",\n"
		   << "  \"milestonesCaptured\": " << ( milestonesCaptured ? "true" : "false" ) << ",\n"
		   << "  \"runtimeCounters\": {\"scheduler\": " << ( diagnostics.schedulerRegistered ? "true" : "false" )
		   << ", \"start\": " << diagnostics.startCallCount << ", \"onTick\": " << diagnostics.onTickCallCount
		   << ", \"pythonCallbacks\": " << diagnostics.pythonCallbackCount << "},\n"
		   << "  \"engine\": {\"active\": " << ( diagnostics.engineKinematicsActive ? "true" : "false" )
		   << ", \"speed\": " << diagnostics.engineParentSpeed
		   << ", \"maximumVelocity\": " << diagnostics.engineMaximumVelocity << "},\n";
	writeArray( "rawPosition", diagnostics.rawPosition, 3, true );
	writeArray( "rawVelocity", diagnostics.rawVelocity, 3, true );
	writeArray( "rawAcceleration", diagnostics.rawAcceleration, 3, true );
	writeArray( "renderRelativePosition", diagnostics.position, 3, true );
	writeArray( "referencePoint", diagnostics.referencePoint, 3, true );
	writeArray( "origin", diagnostics.origin, 3, true );
	writeArray( "rotation", diagnostics.rotation, 4, true );
	writeArray( "gotoPoint", diagnostics.gotoPoint, 3, true );
	writeArray( "engineAcceleration", diagnostics.engineParentAcceleration, 3, true );
	output << "  \"validationPassed\": " << ( diagnostics.motionValid && milestonesCaptured ? "true" : "false" )
		   << "\n}\n";
	std::cout << "Ballpark motion contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteBallparkOrbitContractJson( const Options& options,
									 const TrinityStandaloneBallparkDiagnostics& diagnostics,
									 bool milestonesCaptured )
{
	const std::string outputPath = CaptureBasePath( options ) + "_ballpark-orbit-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-11B native Frontier orbit\",\n"
		   << "  \"solver\": \"frontier-new\",\n"
		   << "  \"clientActivationVerified\": false,\n"
		   << "  \"referenceFrame\": \"" << BallparkFrameName( options.ballparkFrame ) << "\",\n"
		   << "  \"csv\": \"" << options.ballparkLogPath << "\",\n"
		   << "  \"directEvolves\": " << diagnostics.directEvolveCount << ",\n"
		   << "  \"commands\": " << diagnostics.commandCount << ",\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"targetBallId\": " << diagnostics.orbitTargetBallId << ",\n"
		   << "  \"followBallId\": " << diagnostics.followBallId << ",\n"
		   << "  \"surfaceRange\": " << diagnostics.followRange << ",\n"
		   << "  \"centerDistance\": " << diagnostics.orbitCenterDistance << ",\n"
		   << "  \"surfaceDistance\": " << diagnostics.orbitSurfaceDistance << ",\n"
		   << "  \"radialVelocity\": " << diagnostics.orbitRadialVelocity << ",\n"
		   << "  \"tangentialVelocity\": " << diagnostics.orbitTangentialVelocity << ",\n"
		   << "  \"accumulatedPhase\": " << diagnostics.orbitAccumulatedPhase << ",\n"
		   << "  \"settledDistance\": {\"minimum\": " << diagnostics.orbitSettledMinimumDistance
		   << ", \"maximum\": " << diagnostics.orbitSettledMaximumDistance << "},\n"
		   << "  \"maximumErrors\": {\"position\": " << diagnostics.maximumRawPositionError
		   << ", \"velocity\": " << diagnostics.maximumRawVelocityError
		   << ", \"acceleration\": " << diagnostics.maximumRawAccelerationError
		   << ", \"curve\": " << diagnostics.maximumCurveError << ", \"root\": " << diagnostics.maximumRootError
		   << ", \"origin\": " << diagnostics.maximumOriginError << "},\n"
		   << "  \"milestonesCaptured\": " << ( milestonesCaptured ? "true" : "false" ) << ",\n";
	writeArray( "targetPosition", diagnostics.orbitTargetPosition, 3, true );
	writeArray( "targetVelocity", diagnostics.orbitTargetVelocity, 3, true );
	writeArray( "orbitalNormal", diagnostics.orbitNormal, 3, true );
	writeArray( "rawPosition", diagnostics.rawPosition, 3, true );
	writeArray( "rawVelocity", diagnostics.rawVelocity, 3, true );
	writeArray( "referencePoint", diagnostics.referencePoint, 3, true );
	writeArray( "origin", diagnostics.origin, 3, true );
	output << "  \"validationPassed\": " << ( diagnostics.orbitValid && milestonesCaptured ? "true" : "false" )
		   << "\n}\n";
	std::cout << "Ballpark orbit contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteBallparkWarpContractJson( const Options& options,
									const TrinityStandaloneBallparkDiagnostics& diagnostics,
									bool milestonesCaptured )
{
	const std::string outputPath = CaptureBasePath( options ) + "_ballpark-warp-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-11C native warp\",\n"
		   << "  \"referenceFrame\": \"" << BallparkFrameName( options.ballparkFrame ) << "\",\n"
		   << "  \"csv\": \"" << options.ballparkLogPath << "\",\n"
		   << "  \"directEvolves\": " << diagnostics.directEvolveCount << ",\n"
		   << "  \"commands\": " << diagnostics.commandCount << ",\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"warpFactor\": " << diagnostics.warpFactor << ",\n"
		   << "  \"minimumRange\": " << std::setprecision( 12 ) << diagnostics.warpMinRange << ",\n"
		   << "  \"totalDistance\": " << std::setprecision( 12 ) << diagnostics.warpTotalDistance << ",\n"
		   << "  \"aligningTicks\": " << diagnostics.warpAligningTicks << ",\n"
		   << "  \"activationEvolve\": " << diagnostics.warpActivationEvolve << ",\n"
		   << "  \"dropoutEvolve\": " << diagnostics.warpDropoutEvolve << ",\n"
		   << "  \"suspension\": {\"observed\": " << ( diagnostics.warpSuspensionObserved ? "true" : "false" )
		   << ", \"violated\": " << ( diagnostics.warpSuspensionViolated ? "true" : "false" )
		   << ", \"restored\": " << ( diagnostics.warpParticipationRestored ? "true" : "false" ) << "},\n"
		   << "  \"maximumErrors\": {\"position\": " << diagnostics.maximumRawPositionError
		   << ", \"velocity\": " << diagnostics.maximumRawVelocityError
		   << ", \"acceleration\": " << diagnostics.maximumRawAccelerationError
		   << ", \"curve\": " << diagnostics.maximumCurveError << ", \"root\": " << diagnostics.maximumRootError
		   << ", \"origin\": " << diagnostics.maximumOriginError << "},\n"
		   << "  \"milestonesCaptured\": " << ( milestonesCaptured ? "true" : "false" ) << ",\n";
	writeArray( "rawPosition", diagnostics.rawPosition, 3, true );
	writeArray( "rawVelocity", diagnostics.rawVelocity, 3, true );
	writeArray( "referencePoint", diagnostics.referencePoint, 3, true );
	writeArray( "origin", diagnostics.origin, 3, true );
	output << "  \"validationPassed\": " << ( diagnostics.warpValid && milestonesCaptured ? "true" : "false" )
		   << "\n}\n";
	std::cout << "Ballpark warp contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteBallparkApproachContractJson( const Options& options,
										const TrinityStandaloneBallparkDiagnostics& diagnostics,
										bool milestonesCaptured )
{
	const std::string outputPath = CaptureBasePath( options ) + "_ballpark-approach-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-11D native approach\",\n"
		   << "  \"referenceFrame\": \"" << BallparkFrameName( options.ballparkFrame ) << "\",\n"
		   << "  \"csv\": \"" << options.ballparkLogPath << "\",\n"
		   << "  \"directEvolves\": " << diagnostics.directEvolveCount << ",\n"
		   << "  \"commands\": " << diagnostics.commandCount << ",\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"followBallId\": " << diagnostics.followBallId << ",\n"
		   << "  \"surfaceRange\": " << std::setprecision( 12 ) << diagnostics.followRange << ",\n"
		   << "  \"minimumCenterDistance\": " << diagnostics.approachMinimumCenterDistance << ",\n"
		   << "  \"finalCenterDistance\": " << diagnostics.approachFinalCenterDistance << ",\n"
		   << "  \"finalSpeed\": " << diagnostics.approachFinalSpeed << ",\n"
		   << "  \"maximumErrors\": {\"position\": " << diagnostics.maximumRawPositionError
		   << ", \"velocity\": " << diagnostics.maximumRawVelocityError
		   << ", \"acceleration\": " << diagnostics.maximumRawAccelerationError
		   << ", \"curve\": " << diagnostics.maximumCurveError << ", \"root\": " << diagnostics.maximumRootError
		   << ", \"origin\": " << diagnostics.maximumOriginError << "},\n"
		   << "  \"milestonesCaptured\": " << ( milestonesCaptured ? "true" : "false" ) << ",\n";
	writeArray( "rawPosition", diagnostics.rawPosition, 3, true );
	writeArray( "rawVelocity", diagnostics.rawVelocity, 3, true );
	writeArray( "referencePoint", diagnostics.referencePoint, 3, true );
	writeArray( "origin", diagnostics.origin, 3, true );
	output << "  \"validationPassed\": " << ( diagnostics.approachValid && milestonesCaptured ? "true" : "false" )
		   << "\n}\n";
	std::cout << "Ballpark approach contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteChaseCameraContractJson( const Options& options, const TrinityStandaloneBallparkDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_chase-camera-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	output << "{\n"
		   << "  \"contract\": \"PL-11A visual chase camera\",\n"
		   << "  \"referenceFrame\": \"chase\",\n"
		   << "  \"updates\": " << diagnostics.chaseCameraUpdates << ",\n"
		   << "  \"travel\": " << diagnostics.chaseCameraTravel << ",\n"
		   << "  \"distance\": {\"minimum\": " << diagnostics.chaseCameraMinimumDistance
		   << ", \"maximum\": " << diagnostics.chaseCameraMaximumDistance << "},\n"
		   << "  \"maximumFocusErrorDegrees\": " << diagnostics.chaseCameraMaximumFocusErrorDegrees << ",\n"
		   << "  \"orbitDegrees\": {\"minimum\": " << diagnostics.chaseCameraMinimumOrbitDegrees
		   << ", \"maximum\": " << diagnostics.chaseCameraMaximumOrbitDegrees << "},\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"validationPassed\": " << ( diagnostics.chaseCameraValid ? "true" : "false" ) << "\n}\n";
	std::cout << "Chase-camera contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteCelestialContractJson( const Options& options, const TrinityStandaloneCelestialDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_celestial-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const auto* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"PL-12 natural celestial Ballpark placement\",\n"
		   << "  \"frameAnchor\": \"promised-land-stargate-observer\",\n"
		   << "  \"referenceFrame\": \"" << BallparkFrameName( options.ballparkFrame ) << "\",\n"
		   << "  \"csv\": \"" << options.celestialLogPath << "\",\n"
		   << "  \"samples\": " << diagnostics.sampleCount << ",\n"
		   << "  \"sunBallId\": " << diagnostics.sunBallId << ",\n"
		   << "  \"planetBallId\": " << diagnostics.planetBallId << ",\n"
		   << "  \"sunMode\": " << diagnostics.sunMode << ",\n"
		   << "  \"planetMode\": " << diagnostics.planetMode << ",\n"
		   << "  \"sunBallRadius\": " << std::setprecision( 12 ) << diagnostics.sunBallRadius << ",\n"
		   << "  \"planetBallRadius\": " << std::setprecision( 12 ) << diagnostics.planetBallRadius << ",\n";
	writeArray( "sunBallPosition", diagnostics.sunBallPosition, 3, true );
	writeArray( "planetBallPosition", diagnostics.planetBallPosition, 3, true );
	writeArray( "sunWorldPosition", diagnostics.sunWorldPosition, 3, true );
	writeArray( "planetWorldPosition", diagnostics.planetWorldPosition, 3, true );
	writeArray( "sunDirection", diagnostics.sunDirection, 3, true );
	output << "  \"lensFlare\": {\"present\": " << ( diagnostics.lensFlarePresent ? "true" : "false" )
		   << ", \"sunSize\": " << std::setprecision( 12 ) << diagnostics.lensFlareSunSize
		   << ", \"expectedSunSize\": " << diagnostics.expectedLensFlareSunSize
		   << ", \"maximumError\": " << diagnostics.maximumLensFlareSunSizeError << "},\n"
		   << "  \"maximumErrors\": {\"sunDirection\": " << diagnostics.maximumSunDirectionError
		   << ", \"sunWorld\": " << diagnostics.maximumSunWorldError
		   << ", \"planetWorld\": " << diagnostics.maximumPlanetWorldError << "},\n"
		   << "  \"linkage\": {\"active\": " << ( diagnostics.linkageActive ? "true" : "false" )
		   << ", \"sceneSunBall\": " << ( diagnostics.sunCurveMatchesSceneSunBall ? "true" : "false" )
		   << ", \"lensFlareCurve\": " << ( diagnostics.sunCurveMatchesLensFlare ? "true" : "false" )
		   << ", \"planetCurve\": " << ( diagnostics.planetCurveAttached ? "true" : "false" )
		   << ", \"sunIdentifiedUniquely\": " << ( diagnostics.sunIdentifiedUniquely ? "true" : "false" ) << "},\n"
		   << "  \"celestialStateExact\": " << ( diagnostics.celestialStateValid ? "true" : "false" ) << ",\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"validationPassed\": " << ( diagnostics.valid ? "true" : "false" ) << "\n}\n";
	std::cout << "Celestial contract report: " << outputPath << "\n";
	return output.good();
}

bool WriteEveGateContractJson( const Options& options, const TrinityStandaloneEveGateDiagnostics& diagnostics )
{
	const std::string outputPath = CaptureBasePath( options ) + "_eve-gate-contract.json";
	if( !EnsureParentDirectory( outputPath ) )
		return false;
	std::ofstream output( outputPath.c_str() );
	if( !output )
		return false;
	auto writeArray = [&]( const char* name, const double* values, size_t count, bool comma ) {
		output << "  \"" << name << "\": [";
		for( size_t i = 0; i < count; ++i )
			output << ( i ? ", " : "" ) << std::setprecision( 12 ) << values[i];
		output << "]" << ( comma ? "," : "" ) << "\n";
	};
	output << "{\n"
		   << "  \"contract\": \"CP-36 EVE Gate landmark\",\n"
		   << "  \"placementPolicy\": \"authored-skybox: camera-anchored graph kept at its authored local "
			  "transform with a constant ball-frame rotation onto the recovered bearing; the navigation ball "
			  "carries the recovered position; no current-client caller recovered\",\n"
		   << "  \"frameAnchor\": \"promised-land-stargate-observer\",\n"
		   << "  \"distanceRatioPolicy\": \"in-system reconstruction default 0.5 (authored far/in-system "
			  "boundary at 0.4, scale completion at 0.5); explicit via --eve-gate-ratio\",\n"
		   << "  \"samples\": " << diagnostics.sampleCount << ",\n"
		   << "  \"meshes\": " << diagnostics.meshCount << ",\n"
		   << "  \"containers\": " << diagnostics.containerCount << ",\n"
		   << "  \"ballId\": " << diagnostics.ballId << ",\n"
		   << "  \"ballMode\": " << diagnostics.ballMode << ",\n"
		   << "  \"ballRadius\": " << std::setprecision( 12 ) << diagnostics.ballRadius << ",\n"
		   << "  \"authoredRadius\": " << diagnostics.authoredRadius << ",\n";
	writeArray( "ballPosition", diagnostics.ballPosition, 3, true );
	writeArray( "worldPosition", diagnostics.worldPosition, 3, true );
	writeArray( "bearingExpected", diagnostics.bearingExpected, 3, true );
	writeArray( "bearingAchieved", diagnostics.bearingAchieved, 3, true );
	output << "  \"maximumBearingError\": " << diagnostics.maximumBearingError << ",\n"
		   << "  \"linkage\": {\"active\": " << ( diagnostics.linkageActive ? "true" : "false" )
		   << ", \"ballStateExact\": " << ( diagnostics.ballStateExact ? "true" : "false" )
		   << ", \"curveAttached\": " << ( diagnostics.curveAttached ? "true" : "false" ) << "},\n"
		   << "  \"trajectoryHash\": \"" << std::hex << diagnostics.trajectoryHash << std::dec << "\",\n"
		   << "  \"validationPassed\": " << ( diagnostics.valid ? "true" : "false" ) << "\n}\n";
	std::cout << "EVE Gate contract report: " << outputPath << "\n";
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
		if( !options.solarAuditReportPath.empty() &&
			( options.sceneFixture != SceneFixture::NewEden || options.shaderTier != ShaderTier::High ||
			  options.qualityRung < QualityRung::Model || options.maxFrames <= 0 ) )
		{
			std::cerr << "--solar-audit-report requires --scene-fixture new-eden --shader-tier high "
						 "a model-or-higher quality rung, and a positive finite --frames count\n";
			return 2;
		}
		if( ( options.sunBodyLayers != SunBodyLayers::All || !options.solarBodyReportPath.empty() ) &&
			options.sceneFixture != SceneFixture::NewEden )
		{
			std::cerr << "Solar body controls require --scene-fixture new-eden\n";
			return 2;
		}
		if( !options.solarBodyReportPath.empty() &&
			( options.shaderTier != ShaderTier::High || options.qualityRung < QualityRung::Model ||
			  options.cameraView != CameraView::Celestials || options.composition != SceneComposition::System ||
			  options.maxFrames <= 0 ) )
		{
			std::cerr << "--solar-body-report requires --scene-fixture new-eden --shader-tier high "
						 "--camera-view celestials --composition system, a model-or-higher quality rung, "
						 "and a positive finite --frames count\n";
			return 2;
		}
		const bool solarHighRequested = options.sunHighLayersExplicit || !options.solarHighReportPath.empty();
		if( solarHighRequested && options.sceneFixture != SceneFixture::NewEden )
		{
			std::cerr << "Solar High controls require --scene-fixture new-eden\n";
			return 2;
		}
		if( !options.solarHighReportPath.empty() &&
			( options.shaderTier != ShaderTier::High || options.qualityRung < QualityRung::Model ||
			  options.cameraView != CameraView::Celestials || options.composition != SceneComposition::System ||
			  options.maxFrames <= 0 ) )
		{
			std::cerr << "--solar-high-report requires --scene-fixture new-eden --shader-tier high "
						 "--camera-view celestials --composition system, a model-or-higher quality rung, "
						 "and a positive finite --frames count\n";
			return 2;
		}
		if( options.solarParticleSeedExplicit && options.solarHighReportPath.empty() )
		{
			std::cerr << "--solar-particle-seed is valid only with --solar-high-report\n";
			return 2;
		}
		if( options.solarParticlePrewarm == SolarParticlePrewarm::NaturalFirstEmission &&
			( options.solarHighReportPath.empty() ||
			  ( options.sunHighLayers != SunHighLayers::Pillar1 &&
				options.sunHighLayers != SunHighLayers::Pillar2 ) ) )
		{
			std::cerr << "Natural solar-particle prewarm requires a solar High report and one isolated pillar\n";
			return 2;
		}
		if( options.solarHighWarmup != 0 &&
			( options.solarHighReportPath.empty() ||
			  options.solarParticlePrewarm == SolarParticlePrewarm::NaturalFirstEmission ) )
		{
			std::cerr << "--solar-high-warmup requires a solar High report and cannot be combined with natural prewarm\n";
			return 2;
		}
		const bool solarOpticsRequested = !options.solarOpticsReportPath.empty() ||
			options.solarEnvironment != SolarEnvironment::Off ||
			options.solarEnvironmentDistance != SolarEnvironmentDistance::Canonical ||
			options.sceneDistanceFog != AuthoredToggle::Authored ||
			options.solarDesaturate != AuthoredToggle::Authored ||
			options.solarOcclusionView != SolarOcclusionView::Unobscured;
		if( solarOpticsRequested &&
			( options.sceneFixture != SceneFixture::NewEden || options.shaderTier != ShaderTier::High ||
			  options.qualityRung < QualityRung::HdrExposure ) )
		{
			std::cerr << "PL-14D solar optics controls require the High-tier New Eden fixture at hdr-exposure or higher\n";
			return 2;
		}
		if( !options.solarOpticsReportPath.empty() && options.maxFrames <= 0 )
		{
			std::cerr << "--solar-optics-report requires a positive finite --frames count\n";
			return 2;
		}
		const bool solarIlluminationRequested = options.solarIlluminationExplicit ||
			options.solarIlluminationViewExplicit || !options.solarIlluminationReportPath.empty();
		if( solarIlluminationRequested &&
			( options.sceneFixture != SceneFixture::NewEden || options.shaderTier != ShaderTier::High ||
			  options.qualityRung < QualityRung::Model ) )
		{
			std::cerr << "PL-14E solar illumination controls require the High-tier New Eden fixture at the model rung or higher\n";
			return 2;
		}
		if( !options.solarIlluminationReportPath.empty() && options.maxFrames <= 0 )
		{
			std::cerr << "--solar-illumination-report requires a positive finite --frames count\n";
			return 2;
		}
		const bool planetIlluminationView =
			options.solarIlluminationView == SolarIlluminationView::PlanetDay ||
			options.solarIlluminationView == SolarIlluminationView::PlanetLimb ||
			options.solarIlluminationView == SolarIlluminationView::PlanetEclipse;
		if( planetIlluminationView && options.planetLayers != PlanetLayers::Surface )
		{
			std::cerr << "Planet illumination views require --planet-layers surface\n";
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
			// Keep the drawable composited without taking focus. Metal-backed
			// windows ordered behind another closing probe can acquire an
			// occlusion-shaped black region in CGWindowListCreateImage.
			[window orderFrontRegardless];
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
		if( options.taa == TaaMode::Auto )
		{
			options.resolvedTaa =
				options.qualityRung >= QualityRung::HdrPost && options.shaderTier == ShaderTier::High ?
				ResolveClientTaa( options.taa, renderWidth, renderHeight ) :
				TaaMode::Off;
		}
		if( options.validateTemporal && options.temporalTest != TemporalTest::Velocity &&
			options.resolvedTaa != TaaMode::High )
		{
			std::cerr << "The backing resolution resolves --taa auto to " << TaaModeName( options.resolvedTaa )
					  << "; temporal acceptance requires explicit --taa high\n";
			[window close];
			return 2;
		}
		std::cerr << "EVE temporal frontend: requested=" << TaaModeName( options.taa )
				  << " resolved=" << TaaModeName( options.resolvedTaa ) << " backing=" << renderWidth << "x"
				  << renderHeight << " motion=" << MotionModeName( options.motion ) << "\n";
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
		if( options.deterministicEvidence &&
			!TrinityStandaloneProbeConfigureDeterministicEvidence( probe, 3430261 ) )
		{
			std::cerr << "Failed to configure deterministic finite-frame evidence\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( !options.solarAuditReportPath.empty() &&
			( !EnsureParentDirectory( options.solarAuditReportPath ) ||
			  !TrinityStandaloneProbeConfigureSolarAudit( probe, options.solarAuditReportPath.c_str() ) ) )
		{
			std::cerr << "Failed to configure the PL-14A solar audit report\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( options.sunBodyLayers != SunBodyLayers::All || !options.solarBodyReportPath.empty() )
		{
			const std::string reportsDirectory = executableDirectory + "/../Reports/";
			if( ( !options.solarBodyReportPath.empty() && !EnsureParentDirectory( options.solarBodyReportPath ) ) ||
				!TrinityStandaloneProbeConfigureSolarBody(
					probe,
					static_cast<int>( options.sunBodyLayers ),
					options.solarBodyReportPath.empty() ? nullptr : options.solarBodyReportPath.c_str(),
					( reportsDirectory + "NewEdenCelestialResources.json" ).c_str(),
					( reportsDirectory + "NewEdenSunGeometry.json" ).c_str(),
					( reportsDirectory + "NewEdenSunGeneratedCmf.sha256" ).c_str() ) )
			{
				std::cerr << "Failed to configure the PL-14B solar body contract\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
		}
		if( solarHighRequested )
		{
			const std::string reportsDirectory = executableDirectory + "/../Reports/";
			if( ( !options.solarHighReportPath.empty() && !EnsureParentDirectory( options.solarHighReportPath ) ) ||
				!TrinityStandaloneProbeConfigureSolarHigh(
					probe,
					static_cast<int>( options.sunHighLayers ),
					options.solarHighReportPath.empty() ? nullptr : options.solarHighReportPath.c_str(),
					( reportsDirectory + "NewEdenSolarHighResources.json" ).c_str(),
					( reportsDirectory + "NewEdenSolarHighGeometry.json" ).c_str(),
					( reportsDirectory + "NewEdenSolarHighGeneratedCmf.json" ).c_str(),
					options.solarHighReportPath.empty() ? 0 : options.solarParticleSeed,
					options.solarParticlePrewarm == SolarParticlePrewarm::NaturalFirstEmission ) )
			{
				std::cerr << "Failed to configure the PL-14C solar High contract\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
		}
		if( solarOpticsRequested )
		{
			if( ( !options.solarOpticsReportPath.empty() &&
				  !EnsureParentDirectory( options.solarOpticsReportPath ) ) ||
				!TrinityStandaloneProbeConfigureSolarOptics(
					probe,
					static_cast<int>( options.solarEnvironment ),
					static_cast<int>( options.solarEnvironmentDistance ),
					options.sceneDistanceFog == AuthoredToggle::Authored,
					options.solarDesaturate == AuthoredToggle::Authored,
					static_cast<int>( options.solarOcclusionView ),
					options.solarOpticsReportPath.empty() ? nullptr : options.solarOpticsReportPath.c_str() ) )
			{
				std::cerr << "Failed to configure the PL-14D solar optics contract\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
		}
		if( solarIlluminationRequested )
		{
			if( ( !options.solarIlluminationReportPath.empty() &&
				  !EnsureParentDirectory( options.solarIlluminationReportPath ) ) ||
				!TrinityStandaloneProbeConfigureSolarIllumination(
					probe,
					static_cast<int>( options.solarIllumination ),
					static_cast<int>( options.solarIlluminationView ),
					options.solarIlluminationReportPath.empty() ? nullptr :
						options.solarIlluminationReportPath.c_str() ) )
			{
				std::cerr << "Failed to configure the PL-14E solar illumination contract\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
		}
		if( options.enableFroxels && !TrinityStandaloneProbeSetFroxelRenderingEnabled( probe, true ) )
		{
			std::cerr << "Failed to enable froxel rendering\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
		if( options.clientKernels &&
			!TrinityStandaloneProbeAuthorizeFroxelLab( probe, options.froxelLabLedger.c_str() ) )
		{
			std::cerr << "Trinity standalone froxel lab authorization failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
#endif

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
												   options.resolvedEngines == EngineMode::Authored ? 1 : 0,
												   static_cast<int>( options.engineView ),
												   options.engineThrottle,
												   options.modelYawDegrees,
												   TaaModeApiValue( options.resolvedTaa ),
												   static_cast<int>( options.taaDebug ),
												   static_cast<int>( options.motion ),
												   ShadowsApiValue( options.resolvedShadows ),
												   AmbientOcclusionApiValue( options.resolvedAmbientOcclusion ),
												   static_cast<int>( options.aoMethod ) ) )
		{
			std::cerr << "TrinityStandaloneProbeCreateEveScene failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( options.solarParticlePrewarm == SolarParticlePrewarm::NaturalFirstEmission &&
			!TrinityStandaloneProbePrewarmSolarParticles( probe ) )
		{
			std::cerr << "PL-14C natural solar-particle prewarm failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( options.solarHighWarmup != 0 &&
			!TrinityStandaloneProbeWarmupSolarHigh( probe, options.solarHighWarmup ) )
		{
			std::cerr << "PL-14C fixed solar High warmup failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( options.qualityRung != QualityRung::Shell )
		{
			if( !options.ballparkLogPath.empty() && !EnsureParentDirectory( options.ballparkLogPath ) )
			{
				std::cerr << "Failed to create the Ballpark log directory\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeSetCelestialAnchor( probe, options.celestialAnchor ) )
			{
				std::cerr << "TrinityStandaloneProbeSetCelestialAnchor failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeSetWarpTarget(
					probe, options.warpTarget == WarpTarget::EveGate ? 1 : 0 ) )
			{
				std::cerr << "TrinityStandaloneProbeSetWarpTarget failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureBallparkEx(
					probe,
					static_cast<int>( options.ballpark ),
					static_cast<int>( options.ballparkFrame ),
					options.orbitSolver == OrbitSolver::New ? 1 : 0,
					options.orbitRange,
					options.ballparkLogPath.empty() ? nullptr : options.ballparkLogPath.c_str() ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureBallparkEx failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeSetJourneyPlanetFinale( probe, options.journeyPlanetFinale ) )
			{
				std::cerr << "TrinityStandaloneProbeSetJourneyPlanetFinale failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !options.celestialLogPath.empty() && !EnsureParentDirectory( options.celestialLogPath ) )
			{
				std::cerr << "Failed to create the celestial Ballpark log directory\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureCelestialBallpark(
					probe,
					static_cast<int>( options.celestialBallpark ),
					options.celestialLogPath.empty() ? nullptr : options.celestialLogPath.c_str() ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureCelestialBallpark failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureEveGateApproach( probe, options.eveGateApproachFrame ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureEveGateApproach failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureEveGate( probe, options.eveGate, options.eveGateRatio ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureEveGate failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureWarpTunnel( probe, options.warpTunnel ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureWarpTunnel failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			if( !TrinityStandaloneProbeConfigureEveGateTravel( probe, options.eveGateTravel ) )
			{
				std::cerr << "TrinityStandaloneProbeConfigureEveGateTravel failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
		}
		if( options.qualityRung != QualityRung::Shell &&
			!TrinityStandaloneProbeConfigureVolumetrics( probe,
												 VolumetricModeApiValue( options.resolvedVolumetrics ),
												 VolumetricQualityApiValue( options.resolvedVolumetricQuality ),
												 options.volumetricSeed,
												 options.froxelTemporal ) )
		{
			std::cerr << "TrinityStandaloneProbeConfigureVolumetrics failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		if( options.validateTemporal &&
			!TrinityStandaloneProbeConfigureTemporalValidation( probe, static_cast<int>( options.temporalTest ) ) )
		{
			std::cerr << "TrinityStandaloneProbeConfigureTemporalValidation failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
		const bool collectExposureDiagnostics = options.validateExposureTone || options.validatePostFinish ||
			options.validateDistortion || options.validateEngines ||
			( options.validateTemporal && options.resolvedTaa != TaaMode::Off ) ||
			options.exposureSequence != ExposureSequence::None || !options.solarOpticsReportPath.empty() ||
			!options.solarIlluminationReportPath.empty();
		// Product capture must not silently add a same-time diagnostic rerender.
		// Tone validation is an explicit contract because it reconfigures bloom and
		// reads both sides of the tone pass; ordinary post-tone/all captures observe
		// the settled frame exactly as rendered.
		const bool captureToneSnapshot = options.qualityRung >= QualityRung::HdrExposure &&
			!options.capturePrefix.empty() && options.solarOpticsReportPath.empty() &&
			options.validateExposureTone;
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
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
		if( ( options.resolvedVolumetrics == VolumetricMode::Froxel ||
			  options.resolvedVolumetrics == VolumetricMode::All ) &&
			( !TrinityStandaloneProbeConfigureSubmissionDiagnostics( probe, true ) ||
			  !WriteAndSyncIncidentPreflight( options ) ) )
		{
			std::cerr << "Failed to persist the incident froxel submission boundary\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}
#endif

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
		bool ballparkMilestonesCaptured = true;
		uint32_t ballparkMilestoneCount = 0;
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
					const bool sunFacing =
						options.exposureSequence == ExposureSequence::DarkToBright ? secondPhase : !secondPhase;
					if( !TrinityStandaloneProbeSetExposureCameraPhase( probe, true, sunFacing ) )
					{
						std::cerr << "Failed to set the RC-10 exposure camera phase\n";
						TrinityStandaloneProbeDestroyDevice( probe );
						[window close];
						return 1;
					}
				}
				bool captureTemporalSample = false;
				if( options.validateTemporal )
				{
					const int remainingFrames = options.maxFrames - renderedFrames;
					switch( options.temporalTest )
					{
					case TemporalTest::Edges:
						captureTemporalSample = remainingFrames <= 4;
						break;
					case TemporalTest::Silk:
					case TemporalTest::Trails:
						captureTemporalSample = remainingFrames <= 2;
						break;
					default:
						captureTemporalSample = remainingFrames == 1;
						break;
					}
				}
				const std::array<std::pair<int, const char*>, 5> ballparkMilestones = { {
					{ 179, "pre-command" },
					{ 359, "acceleration" },
					{ 719, "overshoot" },
					{ 1019, "reversal" },
					{ 1199, "final" },
				} };
				const std::array<std::pair<int, const char*>, 6> orbitMilestones = { {
					{ 179, "pre-command" },
					{ 359, "acceleration" },
					{ 1319, "quarter" },
					{ 1979, "half" },
					{ 2879, "three-quarter" },
					{ 3779, "full" },
				} };
				// Warp phase frames follow the PL-11C corpus: command at tick 3,
				// eight aligning ticks, activation at evolve 11, the short cruise
				// crest near evolve 16, deceleration through dropout at evolve 31.
				const std::array<std::pair<int, const char*>, 6> warpMilestones = { {
					{ 179, "pre-command" },
					{ 599, "aligning" },
					{ 719, "activation" },
					{ 1019, "cruise" },
					{ 1799, "deceleration" },
					{ 3779, "arrival" },
				} };
				// Approach phase frames follow the PL-11D corpus: command at
				// tick 3, full thrust through tick 9, range crossing near tick
				// 10, deepest overshoot near tick 12, settled under 1 m/s by
				// tick 34, terminal station-keeping.
				const std::array<std::pair<int, const char*>, 6> approachMilestones = { {
					{ 179, "pre-command" },
					{ 539, "acceleration" },
					{ 659, "crossing" },
					{ 779, "overshoot" },
					{ 2039, "settling" },
					{ 3779, "station-keeping" },
				} };
				const char* ballparkMilestone = nullptr;
				if( options.validateBallparkMotion || options.validateBallparkOrbit || options.validateBallparkWarp ||
					options.validateBallparkApproach )
				{
					const auto captureMilestone = [&]( const auto& milestones ) {
						for( const auto& milestone : milestones )
							if( renderedFrames == milestone.first )
								ballparkMilestone = milestone.second;
					};
					if( options.validateBallparkOrbit )
						captureMilestone( orbitMilestones );
					else if( options.validateBallparkWarp )
						captureMilestone( warpMilestones );
					else if( options.validateBallparkApproach )
						captureMilestone( approachMilestones );
					else
					{
						captureMilestone( ballparkMilestones );
					}
				}
				const bool captureSequenceFrame = !options.capturePrefix.empty() &&
					( ( options.captureEvery > 0 && renderedFrames >= options.captureStartFrame &&
						( renderedFrames - options.captureStartFrame ) % options.captureEvery == 0 ) ||
					  std::binary_search(
						  options.captureFrames.begin(), options.captureFrames.end(), renderedFrames ) );
				const int captureProducts = ( ballparkMilestone || captureSequenceFrame ) ? kCaptureColor :
					captureTemporalSample ?
					kCaptureTemporalSample :
					( options.maxFrames > 0 && renderedFrames + 1 == options.maxFrames ?
						  RenderProductApiValue( options.renderProduct == RenderProduct::All ? RenderProduct::Color :
																							   options.renderProduct ) :
						  0 );
				if( !options.solarIlluminationReportPath.empty() &&
					!TrinityStandaloneProbeSetSolarIlluminationCaptureRequested(
						probe, captureSequenceFrame ) )
				{
					std::cerr << "Failed to mark the PL-14E capture frame\n";
					TrinityStandaloneProbeDestroyDevice( probe );
					[window close];
					return 1;
				}
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
				if( ballparkMilestone )
				{
					const std::string path = CaptureBasePath( options ) + "_ballpark-" + ballparkMilestone + ".png";
					const bool captured = EnsureParentDirectory( path ) && CaptureProbeProductPng( probe, path );
					ballparkMilestonesCaptured = captured && ballparkMilestonesCaptured;
					ballparkMilestoneCount += captured ? 1u : 0u;
				}
				if( captureSequenceFrame )
				{
					char frameSuffix[32];
					std::snprintf( frameSuffix, sizeof frameSuffix, "_frame-%06d.png", renderedFrames );
					const std::string path = CaptureBasePath( options ) + frameSuffix;
					// One-time in-frame diagnostics (the dynamic-reflection
					// runtime contract) reuse the readback target and can
					// stomp a single frame; report and skip rather than abort.
					if( !EnsureParentDirectory( path ) || !CaptureProbeProductPng( probe, path ) )
					{
						std::cerr << "Frame-sequence capture skipped at frame " << renderedFrames << "\n";
					}
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

		TrinityStandaloneBallparkDiagnostics ballparkDiagnostics;
		bool ballparkValidationSucceeded = true;
		bool ballparkReportSucceeded = true;
		bool ballparkMotionValidationSucceeded = true;
		bool ballparkMotionReportSucceeded = true;
		bool ballparkOrbitValidationSucceeded = true;
		bool ballparkOrbitReportSucceeded = true;
		bool ballparkWarpValidationSucceeded = true;
		bool ballparkWarpReportSucceeded = true;
		bool ballparkApproachValidationSucceeded = true;
		bool ballparkApproachReportSucceeded = true;
		bool chaseCameraValidationSucceeded = true;
		bool chaseCameraReportSucceeded = true;
		bool encodedBallparkColorEqual = false;
		bool encodedBallparkDepthEqual = false;
		if( options.validateBallpark )
		{
			const bool validationRan = TrinityStandaloneProbeValidateBallpark( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			const bool capturesWritten =
				CaptureBallparkValidationPngs( probe, options, encodedBallparkColorEqual, encodedBallparkDepthEqual );
			ballparkValidationSucceeded = validationRan && diagnosticsRead && ballparkDiagnostics.valid &&
				capturesWritten && encodedBallparkColorEqual && encodedBallparkDepthEqual;
			ballparkReportSucceeded = WriteBallparkContractJson(
				options, ballparkDiagnostics, encodedBallparkColorEqual, encodedBallparkDepthEqual );
		}
		if( options.validateBallparkMotion )
		{
			const bool validationRan = TrinityStandaloneProbeValidateBallparkMotion( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			ballparkMilestonesCaptured = ballparkMilestonesCaptured && ballparkMilestoneCount == 5;
			ballparkMotionValidationSucceeded =
				validationRan && diagnosticsRead && ballparkDiagnostics.motionValid && ballparkMilestonesCaptured;
			ballparkMotionReportSucceeded =
				WriteBallparkMotionContractJson( options, ballparkDiagnostics, ballparkMilestonesCaptured );
		}
		if( options.validateBallparkOrbit )
		{
			const bool validationRan = TrinityStandaloneProbeValidateBallparkOrbit( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			ballparkMilestonesCaptured = ballparkMilestonesCaptured && ballparkMilestoneCount == 6;
			ballparkOrbitValidationSucceeded =
				validationRan && diagnosticsRead && ballparkDiagnostics.orbitValid && ballparkMilestonesCaptured;
			ballparkOrbitReportSucceeded =
				WriteBallparkOrbitContractJson( options, ballparkDiagnostics, ballparkMilestonesCaptured );
		}
		if( options.validateBallparkWarp )
		{
			const bool validationRan = TrinityStandaloneProbeValidateBallparkWarp( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			ballparkMilestonesCaptured = ballparkMilestonesCaptured && ballparkMilestoneCount == 6;
			ballparkWarpValidationSucceeded =
				validationRan && diagnosticsRead && ballparkDiagnostics.warpValid && ballparkMilestonesCaptured;
			ballparkWarpReportSucceeded =
				WriteBallparkWarpContractJson( options, ballparkDiagnostics, ballparkMilestonesCaptured );
		}
		if( options.validateBallparkApproach )
		{
			const bool validationRan = TrinityStandaloneProbeValidateBallparkApproach( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			ballparkMilestonesCaptured = ballparkMilestonesCaptured && ballparkMilestoneCount == 6;
			ballparkApproachValidationSucceeded =
				validationRan && diagnosticsRead && ballparkDiagnostics.approachValid && ballparkMilestonesCaptured;
			ballparkApproachReportSucceeded =
				WriteBallparkApproachContractJson( options, ballparkDiagnostics, ballparkMilestonesCaptured );
		}
		if( options.validateChaseCamera )
		{
			const bool validationRan = TrinityStandaloneProbeValidateChaseCamera( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetBallparkDiagnostics( probe, &ballparkDiagnostics );
			chaseCameraValidationSucceeded = validationRan && diagnosticsRead && ballparkDiagnostics.chaseCameraValid;
			chaseCameraReportSucceeded = WriteChaseCameraContractJson( options, ballparkDiagnostics );
		}
		TrinityStandaloneCelestialDiagnostics celestialDiagnostics;
		bool celestialValidationSucceeded = true;
		bool celestialReportSucceeded = true;
		if( options.validateCelestialBallpark )
		{
			const bool validationRan = TrinityStandaloneProbeValidateCelestialBallpark( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetCelestialDiagnostics( probe, &celestialDiagnostics );
			celestialValidationSucceeded = validationRan && diagnosticsRead && celestialDiagnostics.valid;
			celestialReportSucceeded = WriteCelestialContractJson( options, celestialDiagnostics );
		}
		TrinityStandaloneEveGateDiagnostics eveGateDiagnostics;
		bool eveGateValidationSucceeded = true;
		bool eveGateReportSucceeded = true;
		if( options.validateEveGate )
		{
			const bool validationRan = TrinityStandaloneProbeValidateEveGate( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetEveGateDiagnostics( probe, &eveGateDiagnostics );
			eveGateValidationSucceeded = validationRan && diagnosticsRead && eveGateDiagnostics.valid;
			eveGateReportSucceeded = WriteEveGateContractJson( options, eveGateDiagnostics );
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
		const bool validateExposureSamples =
			options.validateExposureTone || options.exposureSequence != ExposureSequence::None;
		const bool exposureValidationSucceeded =
			!validateExposureSamples || EvaluateExposureSamples( options, exposureSamples, exposureValidationStats );
		std::string temporalValidationStats = "not-requested";
		TrinityStandaloneTemporalValidation temporalValidation;
		bool temporalValidationSucceeded = true;
		if( options.validateTemporal )
		{
			const bool quantitativeEvaluationPassed = TrinityStandaloneProbeEvaluateTemporalValidation( probe );
			const bool quantitativeDiagnosticsRead =
				TrinityStandaloneProbeGetTemporalValidation( probe, &temporalValidation );
			const bool quantitativePassed =
				quantitativeEvaluationPassed && quantitativeDiagnosticsRead && temporalValidation.valid;
			bool nativeContractPassed = true;
			std::string nativeSummary = "taa=off";
			if( options.resolvedTaa != TaaMode::Off )
			{
				nativeContractPassed =
					EvaluateTemporalSamples( options, exposureSamples, renderWidth, renderHeight, nativeSummary );
			}
			std::ostringstream stats;
			stats << "validation=" << ( quantitativePassed && nativeContractPassed ? "pass" : "fail" )
				  << " test=" << TemporalTestName( options.temporalTest )
				  << " samples=" << temporalValidation.sampleCount
				  << " velocityInterior=" << temporalValidation.velocityInteriorSamples
				  << " velocityMeanError=" << temporalValidation.velocityMeanError
				  << " velocityP99Error=" << temporalValidation.velocityP99Error
				  << " velocityMaxError=" << temporalValidation.velocityMaximumError
				  << " edgePixels=" << temporalValidation.edgePixels
				  << " edgeRatio=" << temporalValidation.edgeVarianceRatio
				  << " transientPixels=" << temporalValidation.currentTransientPixels
				  << " previousOnly=" << temporalValidation.previousOnlyTransientPixels
				  << " cooldownActive=" << temporalValidation.cooldownActivePixels
				  << " cooldownOverlap=" << temporalValidation.cooldownOverlapPixels
				  << " residualRatio=" << temporalValidation.transientResidualRatio << " native={" << nativeSummary
				  << "}";
			temporalValidationStats = stats.str();
			temporalValidationSucceeded = quantitativePassed && nativeContractPassed;
		}
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
					toneValidationSucceeded =
						CompareToneValidations( offToneValidation, toneValidation, toneComparisonStats );
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
		TrinityStandaloneVolumetricDiagnostics volumetricDiagnostics;
		bool volumetricValidationSucceeded = true;
		if( options.validateVolumetrics )
		{
			const bool validationRan = TrinityStandaloneProbeValidateVolumetrics( probe );
			const bool diagnosticsRead =
				TrinityStandaloneProbeGetVolumetricDiagnostics( probe, &volumetricDiagnostics );
			volumetricValidationSucceeded = validationRan && diagnosticsRead && volumetricDiagnostics.valid;
		}
		else if( options.resolvedVolumetrics == VolumetricMode::Froxel ||
				 options.resolvedVolumetrics == VolumetricMode::All )
		{
			if( TrinityStandaloneProbeGetVolumetricDiagnostics( probe, &volumetricDiagnostics ) )
			{
				std::cerr << "EVE froxel diagnostics: volume=" << volumetricDiagnostics.froxelWidth << "x"
						  << volumetricDiagnostics.froxelHeight << "x" << volumetricDiagnostics.froxelDepth
						  << " threadgroup=" << volumetricDiagnostics.raymarchThreadGroupX << "x"
						  << volumetricDiagnostics.raymarchThreadGroupY << "x"
						  << volumetricDiagnostics.raymarchThreadGroupZ << " dispatch="
						  << volumetricDiagnostics.raymarchDispatchX << "x"
						  << volumetricDiagnostics.raymarchDispatchY << "x"
						  << volumetricDiagnostics.raymarchDispatchZ << " stages="
						  << ( volumetricDiagnostics.calculateSucceeded ? "calculate " : "" )
						  << ( volumetricDiagnostics.temporalFilterSucceeded ? "filter " : "" )
						  << ( volumetricDiagnostics.raymarchSucceeded ? "raymarch " : "" )
						  << ( volumetricDiagnostics.applySucceeded ? "apply" : "" ) << "\n";
			}
		}
		TrinityStandaloneEngineDiagnostics engineDiagnostics;
		bool engineValidationSucceeded = true;
		if( options.validateEngines )
		{
			const bool validationRan = TrinityStandaloneProbeValidateEngines( probe );
			const bool diagnosticsRead = TrinityStandaloneProbeGetEngineDiagnostics( probe, &engineDiagnostics );
			engineValidationSucceeded = validationRan && diagnosticsRead && engineDiagnostics.valid;
		}
		bool exposureReportsSucceeded = true;
		if( collectExposureDiagnostics )
		{
			exposureReportsSucceeded =
				WriteExposureCsv( CaptureBasePath( options ) + "_exposure.csv", exposureSamples );
		}
		if( captureToneSnapshot )
		{
			exposureReportsSucceeded = WriteToneContractJson( options,
															  toneValidation,
															  exposureValidationStats,
															  options.resolvedDynamicExposure,
															  "_tone-contract.json" ) &&
				exposureReportsSucceeded;
			if( runMatchedToneComparison )
			{
				exposureReportsSucceeded = WriteToneContractJson( options,
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
		if( options.validateVolumetrics )
		{
			exposureReportsSucceeded =
				WriteVolumetricContractJson( options, volumetricDiagnostics ) && exposureReportsSucceeded;
		}
		if( options.validateEngines )
		{
			exposureReportsSucceeded =
				WriteEngineContractJson( options, engineDiagnostics ) && exposureReportsSucceeded;
		}
		if( options.validateTemporal )
		{
			exposureReportsSucceeded = WriteTemporalContractJson( options,
																  exposureSamples,
																  temporalValidation,
																  temporalValidationStats,
																  temporalValidationSucceeded ) &&
				exposureReportsSucceeded;
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
			if( validateExposureSamples )
			{
				productStats.emplace_back( "exposureValidation", exposureValidationStats );
			}
			if( options.validateTemporal )
			{
				productStats.emplace_back( "temporalValidation", temporalValidationStats );
			}
			if( options.validateBallpark )
			{
				std::ostringstream stats;
				stats << "classes=" << ballparkDiagnostics.registeredClassCount
					  << " evolves=" << ballparkDiagnostics.directEvolveCount
					  << " originUpdates=" << ballparkDiagnostics.originUpdateCount << " offColor=" << std::hex
					  << ballparkDiagnostics.offColorHash << " staticColor=" << ballparkDiagnostics.staticColorHash
					  << " offDepth=" << ballparkDiagnostics.offDepthHash
					  << " staticDepth=" << ballparkDiagnostics.staticDepthHash << std::dec
					  << " world=" << ( ballparkDiagnostics.worldMatricesEqual ? "identical" : "different" )
					  << " encodedColor=" << ( encodedBallparkColorEqual ? "identical" : "different" )
					  << " encodedDepth=" << ( encodedBallparkDepthEqual ? "identical" : "different" )
					  << " validation=" << ( ballparkValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "ballparkValidation", stats.str() );
			}
			if( options.validateBallparkMotion )
			{
				std::ostringstream stats;
				stats << "frame=" << BallparkFrameName( options.ballparkFrame )
					  << " evolves=" << ballparkDiagnostics.directEvolveCount
					  << " command=" << ballparkDiagnostics.commandCount << " trajectory=" << std::hex
					  << ballparkDiagnostics.trajectoryHash << std::dec
					  << " positionError=" << ballparkDiagnostics.maximumRawPositionError
					  << " velocityError=" << ballparkDiagnostics.maximumRawVelocityError
					  << " rootError=" << ballparkDiagnostics.maximumRootError
					  << " milestones=" << ballparkMilestoneCount
					  << " validation=" << ( ballparkMotionValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "ballparkMotionValidation", stats.str() );
			}
			if( options.validateBallparkOrbit )
			{
				std::ostringstream stats;
				stats << "frame=" << BallparkFrameName( options.ballparkFrame )
					  << " evolves=" << ballparkDiagnostics.directEvolveCount << " trajectory=" << std::hex
					  << ballparkDiagnostics.trajectoryHash << std::dec
					  << " phase=" << ballparkDiagnostics.orbitAccumulatedPhase
					  << " distance=" << ballparkDiagnostics.orbitCenterDistance
					  << " milestones=" << ballparkMilestoneCount
					  << " validation=" << ( ballparkOrbitValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "ballparkOrbitValidation", stats.str() );
			}
			if( options.validateBallparkWarp )
			{
				std::ostringstream stats;
				stats << "frame=" << BallparkFrameName( options.ballparkFrame )
					  << " evolves=" << ballparkDiagnostics.directEvolveCount << " trajectory=" << std::hex
					  << ballparkDiagnostics.trajectoryHash << std::dec
					  << " align=" << ballparkDiagnostics.warpAligningTicks
					  << " activation=" << ballparkDiagnostics.warpActivationEvolve
					  << " dropout=" << ballparkDiagnostics.warpDropoutEvolve
					  << " leg=" << ballparkDiagnostics.warpTotalDistance << " milestones=" << ballparkMilestoneCount
					  << " validation=" << ( ballparkWarpValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "ballparkWarpValidation", stats.str() );
			}
			if( options.validateBallparkApproach )
			{
				std::ostringstream stats;
				stats << "frame=" << BallparkFrameName( options.ballparkFrame )
					  << " evolves=" << ballparkDiagnostics.directEvolveCount << " trajectory=" << std::hex
					  << ballparkDiagnostics.trajectoryHash << std::dec
					  << " min-center=" << ballparkDiagnostics.approachMinimumCenterDistance
					  << " final-center=" << ballparkDiagnostics.approachFinalCenterDistance
					  << " final-speed=" << ballparkDiagnostics.approachFinalSpeed
					  << " milestones=" << ballparkMilestoneCount
					  << " validation=" << ( ballparkApproachValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "ballparkApproachValidation", stats.str() );
			}
			if( options.validateCelestialBallpark )
			{
				std::ostringstream stats;
				stats << "samples=" << celestialDiagnostics.sampleCount
					  << " directionError=" << celestialDiagnostics.maximumSunDirectionError
					  << " worldErrors=" << celestialDiagnostics.maximumSunWorldError << "/"
					  << celestialDiagnostics.maximumPlanetWorldError
					  << " sunSize=" << celestialDiagnostics.lensFlareSunSize << " trajectory=" << std::hex
					  << celestialDiagnostics.trajectoryHash << std::dec
					  << " validation=" << ( celestialValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "celestialBallparkValidation", stats.str() );
			}
			if( options.validateChaseCamera )
			{
				std::ostringstream stats;
				stats << "updates=" << ballparkDiagnostics.chaseCameraUpdates
					  << " travel=" << ballparkDiagnostics.chaseCameraTravel
					  << " distance=" << ballparkDiagnostics.chaseCameraMinimumDistance << ".."
					  << ballparkDiagnostics.chaseCameraMaximumDistance
					  << " focusError=" << ballparkDiagnostics.chaseCameraMaximumFocusErrorDegrees
					  << " orbit=" << ballparkDiagnostics.chaseCameraMinimumOrbitDegrees << ".."
					  << ballparkDiagnostics.chaseCameraMaximumOrbitDegrees
					  << " validation=" << ( chaseCameraValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "chaseCameraValidation", stats.str() );
			}
			if( captureToneSnapshot )
			{
				std::ostringstream stats;
				stats << "preHash=" << std::hex << std::setw( 16 ) << std::setfill( '0' )
					  << toneValidation.preTonemapHash << " postHash=" << std::setw( 16 )
					  << toneValidation.postTonemapHash << std::dec << " fullyBlack=" << toneValidation.fullyBlackPixels
					  << " fullyWhite=" << toneValidation.fullyWhitePixels
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
					  << distortionDiagnostics.authoredFinalHash << std::dec
					  << " map=" << distortionDiagnostics.mapWidth << "x" << distortionDiagnostics.mapHeight
					  << " nonNeutral=" << distortionDiagnostics.nonNeutralPixels
					  << " affectedBounds=" << distortionDiagnostics.affectedMinX << ","
					  << distortionDiagnostics.affectedMinY << "-" << distortionDiagnostics.affectedMaxX << ","
					  << distortionDiagnostics.affectedMaxY
					  << " validation=" << ( distortionValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "distortionValidation", stats.str() );
			}
			if( options.validateVolumetrics )
			{
				std::ostringstream stats;
				stats << "mode=" << VolumetricModeName( options.resolvedVolumetrics )
					  << " quality=" << VolumetricQualityName( options.resolvedVolumetricQuality )
					  << " seed=" << options.volumetricSeed << " local=" << volumetricDiagnostics.localRenderableCount
					  << " batches=" << volumetricDiagnostics.localBatchCount
					  << " slices=" << volumetricDiagnostics.volumeWidth << "x" << volumetricDiagnostics.volumeHeight
					  << "x" << volumetricDiagnostics.volumeLayers << " volumeHash=" << std::hex << std::setw( 16 )
					  << std::setfill( '0' ) << volumetricDiagnostics.volumeProductHash << std::dec
					  << " nonzero=" << volumetricDiagnostics.volumeProductNonzeroPixels
					  << " lightmapUpdates=" << volumetricDiagnostics.totalLocalLightmapUpdates
					  << " lightmapSettled=" << ( volumetricDiagnostics.localLightmapSettled ? "true" : "false" )
					  << " shadowBatches=" << volumetricDiagnostics.localShadowBatchCount
					  << " outputCopy=" << ( volumetricDiagnostics.localOutputCopySucceeded ? "true" : "false" )
					  << " validation=" << ( volumetricValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "volumetricValidation", stats.str() );
			}
			if( options.validateEngines )
			{
				std::ostringstream stats;
				stats << "view=" << EngineViewName( options.engineView ) << " throttle=" << engineDiagnostics.throttle
					  << " nativeIntensity=" << engineDiagnostics.nativeIntensity
					  << " intensity=" << engineDiagnostics.intensity << " boosters=" << engineDiagnostics.boosterCount
					  << " plumeBatches=" << engineDiagnostics.plumeBatchCount
					  << " plumeInstances=" << engineDiagnostics.plumeInstanceCount
					  << " glows=" << engineDiagnostics.glowSubmissionCount
					  << " trailBatches=" << engineDiagnostics.trailBatchCount
					  << " trailInstances=" << engineDiagnostics.trailInstanceCount
					  << " trailLength=" << engineDiagnostics.trailLength
					  << " lights=" << engineDiagnostics.lightSubmissionCount
					  << " validation=" << ( engineValidationSucceeded ? "pass" : "fail" );
				productStats.emplace_back( "engineValidation", stats.str() );
			}
			if( !options.capturePrefix.empty() )
			{
				ProcessEvents( window );
				if( options.validateEngines && options.renderProduct == RenderProduct::Window )
				{
					productStats.emplace_back(
						"windowCapture",
						"skipped-after-named-product-validation; use a separate no-readback visual run" );
				}
				else if( options.renderProduct == RenderProduct::All )
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
					if( options.resolvedTaa != TaaMode::Off )
					{
						diagnosticProducts.push_back( RenderProduct::Velocity );
						diagnosticProducts.push_back( RenderProduct::TaaInput );
						diagnosticProducts.push_back( RenderProduct::TaaOutput );
						diagnosticProducts.push_back( RenderProduct::TaaCooldown );
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
					if( options.resolvedShadows == Shadows::Raytraced )
					{
						TrinityStandaloneRaytracedShadowDiagnostics diagnostics;
						if( !TrinityStandaloneProbeGetRaytracedShadowDiagnostics(
								probe, &diagnostics ) )
						{
							captureSucceeded = false;
						}
						else
						{
							std::ostringstream stats;
							stats << "geometry=" << ( diagnostics.geometryPresent ? "ready" : "missing" )
								  << " dispatch=" << ( diagnostics.dispatchSucceeded ? "success" : "failed" )
								  << " dispatchCount=" << diagnostics.dispatchCount
								  << " denoise="
								  << ( !diagnostics.denoiserRequested || diagnostics.denoiserSucceeded ?
										 "success" : "failed" )
								  << " denoiserCount=" << diagnostics.denoiserCount
								  << " maskGenerated=" << ( diagnostics.maskGenerated ? "true" : "false" )
								  << " maskFormat=" << diagnostics.maskFormat
								  << " mask=" << diagnostics.maskWidth << "x" << diagnostics.maskHeight;
							productStats.emplace_back( "raytracedShadowRuntime", stats.str() );
						}
					}
					else if( options.resolvedShadows != Shadows::Off )
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
					if( options.resolvedVolumetrics == VolumetricMode::Silk ||
						options.resolvedVolumetrics == VolumetricMode::All )
					{
						diagnosticProducts.push_back( RenderProduct::VolumeSlices );
					}
					if( options.resolvedVolumetrics == VolumetricMode::Froxel ||
						options.resolvedVolumetrics == VolumetricMode::All )
					{
						diagnosticProducts.push_back( RenderProduct::FroxelFog );
						diagnosticProducts.push_back( RenderProduct::MieEnvironment );
					}
					for( RenderProduct product : diagnosticProducts )
					{
						const int64_t captureTime =
							static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
						const int frozenProduct = RenderProductApiValue( product ) | kCaptureFreezeScene;
						bool productRendered = captureSucceeded &&
							TrinityStandaloneProbeRenderFrame(
												   probe, qualityRung, captureTime, captureTime, frozenProduct );
						if( productRendered )
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
					if( options.renderProduct != RenderProduct::Window &&
						options.renderProduct != RenderProduct::Color )
					{
						const int64_t captureTime =
							static_cast<int64_t>( std::max( renderedFrames - 1, 0 ) ) * kFrameTime;
						const int frozenProduct =
							RenderProductApiValue( options.renderProduct ) | kCaptureFreezeScene;
						captureSucceeded = TrinityStandaloneProbeRenderFrame(
							probe,
							qualityRung,
							captureTime,
							captureTime,
							frozenProduct );
						if( captureSucceeded )
						{
							captureSucceeded = TrinityStandaloneProbeRenderFrame(
								probe, qualityRung, captureTime, captureTime, frozenProduct );
						}
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
					if( options.resolvedShadows == Shadows::Raytraced )
					{
						TrinityStandaloneRaytracedShadowDiagnostics diagnostics;
						if( !TrinityStandaloneProbeGetRaytracedShadowDiagnostics(
								probe, &diagnostics ) )
						{
							captureSucceeded = false;
						}
						else
						{
							std::ostringstream stats;
							stats << "geometry=" << ( diagnostics.geometryPresent ? "ready" : "missing" )
								  << " dispatch=" << ( diagnostics.dispatchSucceeded ? "success" : "failed" )
								  << " dispatchCount=" << diagnostics.dispatchCount
								  << " denoise="
								  << ( !diagnostics.denoiserRequested || diagnostics.denoiserSucceeded ?
										 "success" : "failed" )
								  << " denoiserCount=" << diagnostics.denoiserCount
								  << " maskGenerated=" << ( diagnostics.maskGenerated ? "true" : "false" )
								  << " maskFormat=" << diagnostics.maskFormat
								  << " mask=" << diagnostics.maskWidth << "x" << diagnostics.maskHeight;
							productStats.emplace_back( "shadowRuntime", stats.str() );
						}
					}
					else
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
							stats << "casterTests=" << casterTests
								  << " acceptedCascades=" << acceptedCascades
								  << " committedBatches=" << committedBatches
								  << " expectedBatches=" << acceptedCascades * 2;
							productStats.emplace_back( "shadowRuntime", stats.str() );
						}
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
#if defined( TRINITY_FROXEL_INCIDENT_LAB )
				if( options.clientKernels )
				{
					const bool incidentReportWritten = WriteAndSyncIncidentReport( options, probe );
					captureSucceeded = incidentReportWritten && captureSucceeded;
				}
#endif
				captureSucceeded = captureSucceeded &&
					WriteCaptureMetadata( options, renderWidth, renderHeight, renderedFrames, productStats );
			}
		}
		bool solarAuditSucceeded = true;
		if( !options.solarAuditReportPath.empty() )
		{
			solarAuditSucceeded = TrinityStandaloneProbeWriteSolarAudit( probe );
			if( solarAuditSucceeded )
			{
				std::cout << "Solar audit report: " << options.solarAuditReportPath << "\n";
			}
		}
		bool solarBodyReportSucceeded = true;
		if( !options.solarBodyReportPath.empty() )
		{
			solarBodyReportSucceeded = TrinityStandaloneProbeWriteSolarBodyReport( probe );
			if( solarBodyReportSucceeded )
			{
				std::cout << "Solar body report: " << options.solarBodyReportPath << "\n";
			}
		}
		bool solarHighReportSucceeded = true;
		if( !options.solarHighReportPath.empty() )
		{
			solarHighReportSucceeded = TrinityStandaloneProbeWriteSolarHighReport( probe );
			if( solarHighReportSucceeded )
			{
				std::cout << "Solar High report: " << options.solarHighReportPath << "\n";
			}
		}
		bool solarOpticsReportSucceeded = true;
		if( !options.solarOpticsReportPath.empty() )
		{
			solarOpticsReportSucceeded = TrinityStandaloneProbeWriteSolarOpticsReport( probe );
			if( solarOpticsReportSucceeded )
			{
				std::cout << "Solar optics report: " << options.solarOpticsReportPath << "\n";
			}
		}
		bool solarIlluminationReportSucceeded = true;
		if( !options.solarIlluminationReportPath.empty() )
		{
			solarIlluminationReportSucceeded =
				TrinityStandaloneProbeWriteSolarIlluminationReport( probe );
			if( solarIlluminationReportSucceeded )
			{
				std::cout << "Solar illumination report: "
						  << options.solarIlluminationReportPath << "\n";
			}
		}
		captureSucceeded = captureSucceeded && solarAuditSucceeded && solarBodyReportSucceeded && solarHighReportSucceeded &&
			solarOpticsReportSucceeded && solarIlluminationReportSucceeded &&
			framePacingSucceeded && compositionValidationSucceeded && exposureValidationSucceeded &&
			toneValidationSucceeded && postFinishValidationSucceeded && distortionValidationSucceeded &&
			volumetricValidationSucceeded && engineValidationSucceeded && temporalValidationSucceeded &&
			ballparkValidationSucceeded && ballparkReportSucceeded && ballparkMotionValidationSucceeded &&
			ballparkMotionReportSucceeded && ballparkOrbitValidationSucceeded && ballparkOrbitReportSucceeded &&
			ballparkWarpValidationSucceeded && ballparkWarpReportSucceeded && ballparkApproachValidationSucceeded &&
			ballparkApproachReportSucceeded && chaseCameraValidationSucceeded && chaseCameraReportSucceeded &&
			celestialValidationSucceeded && celestialReportSucceeded && eveGateValidationSucceeded &&
			eveGateReportSucceeded && exposureReportsSucceeded;
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
