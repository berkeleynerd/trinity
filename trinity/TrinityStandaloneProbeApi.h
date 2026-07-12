// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>

struct TrinityStandalonePostProcessDiagnostics
{
	int64_t realTime = 0;
	int64_t simTime = 0;
	bool dynamicExposureActive = false;
	bool histogramCreated = false;
	bool histogramMerged = false;
	bool exposureMeasured = false;
	bool tonemappingSucceeded = false;
	bool bloomActive = false;
	bool useNewBloom = false;
	bool bloomHighPassSucceeded = false;
	bool bloomBlurHorizontalSucceeded = false;
	bool bloomBlurVerticalSucceeded = false;
	bool bloomSucceeded = false;
	bool filmGrainActive = false;
	bool filmGrainSucceeded = false;
	bool taaActive = false;
	bool taaReset = false;
	bool taaAccumulationSucceeded = false;
	bool taaCopySucceeded = false;
	uint32_t sourceWidth = 0;
	uint32_t sourceHeight = 0;
	uint32_t sourceFormat = 0;
	uint32_t postTonemapWidth = 0;
	uint32_t postTonemapHeight = 0;
	uint32_t postTonemapFormat = 0;
	uint32_t bloomWidth = 0;
	uint32_t bloomHeight = 0;
	uint32_t bloomFormat = 0;
	uint32_t finalWidth = 0;
	uint32_t finalHeight = 0;
	uint32_t finalFormat = 0;
	uint32_t taaFrameIndex = 0;
	uint32_t taaQuality = 0;
	uint32_t taaDebugMode = 0;
	uint32_t velocityWidth = 0;
	uint32_t velocityHeight = 0;
	uint32_t velocityFormat = 0;
	uint32_t opaqueWidth = 0;
	uint32_t opaqueHeight = 0;
	uint32_t opaqueFormat = 0;
	uint32_t taaAccumulationWidth = 0;
	uint32_t taaAccumulationHeight = 0;
	uint32_t taaAccumulationFormat = 0;
	uint32_t taaCooldownWidth = 0;
	uint32_t taaCooldownHeight = 0;
	uint32_t taaCooldownFormat = 0;
	uint32_t histogram[65] = {};
	float exposure[8] = {};
	float minBrightness = 0.0f;
	float maxBrightness = 0.0f;
	float increaseSpeed = 0.0f;
	float decreaseSpeed = 0.0f;
	float minLuminance = 0.0f;
	float maxLuminance = 0.0f;
	float exposureInfluence = 0.0f;
	float exposureMiddleValue = 0.0f;
	float exposureAdjustment = 0.0f;
	float minExposure = 0.0f;
	float maxExposure = 0.0f;
	float shoulderStrength = 0.0f;
	float linearStrength = 0.0f;
	float linearAngle = 0.0f;
	float toeStrength = 0.0f;
	float toeNumerator = 0.0f;
	float toeDenominator = 0.0f;
	float whiteScale = 0.0f;
	float outputGamma = 0.0f;
	int32_t tonemappingMethod = -1;
	float bloomLuminanceThreshold = 0.0f;
	float bloomLuminanceScale = 0.0f;
	float bloomBrightness = 0.0f;
	bool bloomExposureDependency = false;
	float bloomGrimeWeight = 0.0f;
	bool filmGrainColored = false;
	float filmGrainColorAmount = 0.0f;
	float filmGrainSize = 0.0f;
	float filmGrainIntensity = 0.0f;
	float filmGrainDensity = 0.0f;
	float filmGrainContrast = 0.0f;
	float filmGrainBrightnessModifier = 0.0f;
	float taaBlendWeight = 0.0f;
	float taaEarlyOutThreshold = 0.0f;
	float taaJitterX = 0.0f;
	float taaJitterY = 0.0f;
	float taaJitterPixelX = 0.0f;
	float taaJitterPixelY = 0.0f;
	uint64_t velocityRawHash = 0;
	uint64_t velocityFinitePixels = 0;
	uint64_t velocityInvalidPixels = 0;
	uint64_t velocityMovingPixels = 0;
	double velocityMeanMagnitude = 0.0;
	double velocityMaximumMagnitude = 0.0;
	uint64_t taaCooldownRawHash = 0;
	uint64_t taaCooldownNonzeroPixels = 0;
	uint32_t taaCooldownMinimum = 0;
	uint32_t taaCooldownMaximum = 0;
};

struct TrinityStandaloneToneValidation
{
	bool valid = false;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t preTonemapFormat = 0;
	uint32_t postTonemapFormat = 0;
	uint64_t preTonemapHash = 0;
	uint64_t postTonemapHash = 0;
	uint64_t fullyBlackPixels = 0;
	uint64_t fullyWhitePixels = 0;
	uint64_t anySaturatedChannelPixels = 0;
	double luminanceP01 = 0.0;
	double luminanceP50 = 0.0;
	double luminanceP99 = 0.0;
	double meanAbsoluteError = 0.0;
	double p999AbsoluteError = 0.0;
	double maximumAbsoluteError = 0.0;
};

struct TrinityStandalonePostFinishValidation
{
	bool valid = false;
	bool bloomValid = false;
	bool filmGrainValid = false;
	uint32_t bloomWidth = 0;
	uint32_t bloomHeight = 0;
	uint32_t bloomFormat = 0;
	uint32_t finalWidth = 0;
	uint32_t finalHeight = 0;
	uint32_t finalFormat = 0;
	uint64_t bloomHash = 0;
	uint64_t postTonemapHash = 0;
	uint64_t finalHash = 0;
	uint64_t bloomNonzeroPixels = 0;
	uint64_t bloomInvalidComponents = 0;
	uint64_t grainChangedPixels = 0;
	uint64_t grainAlphaChangedPixels = 0;
	double bloomMinimumLuminance = 0.0;
	double bloomMeanLuminance = 0.0;
	double bloomMaximumLuminance = 0.0;
	double grainMeanAbsoluteError = 0.0;
	double grainP99AbsoluteError = 0.0;
	double grainMaximumAbsoluteError = 0.0;
};

struct TrinityStandaloneDistortionDiagnostics
{
	bool valid = false;
	bool enabled = false;
	bool mapCreated = false;
	bool backgroundBatches = false;
	bool foregroundBatches = false;
	bool copySucceeded = false;
	bool compositeSucceeded = false;
	uint32_t submittedBatches = 0;
	uint32_t submittedIndices = 0;
	uint32_t backgroundApplications = 0;
	uint32_t foregroundApplications = 0;
	uint32_t mapWidth = 0;
	uint32_t mapHeight = 0;
	uint32_t mapFormat = 0;
	uint64_t mapHash = 0;
	uint64_t neutralPixels = 0;
	uint64_t nonNeutralPixels = 0;
	uint64_t saturatedPixels = 0;
	uint8_t minimumRed = 255;
	uint8_t maximumRed = 0;
	uint8_t minimumGreen = 255;
	uint8_t maximumGreen = 0;
	uint32_t affectedMinX = 0;
	uint32_t affectedMinY = 0;
	uint32_t affectedMaxX = 0;
	uint32_t affectedMaxY = 0;
	uint64_t offPreTonemapHash = 0;
	uint64_t authoredPreTonemapHash = 0;
	uint64_t offFinalHash = 0;
	uint64_t authoredFinalHash = 0;
};

struct TrinityStandaloneVolumetricDiagnostics
{
	bool valid = false;
	bool validationAttempted = false;
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
	bool localLightmapSettled = false;
	uint32_t mode = 0;
	uint32_t quality = 0;
	uint32_t seed = 0;
	uint32_t localRenderableCount = 0;
	uint32_t froxelSettingsCount = 0;
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
	uint64_t volumeProductHash = 0;
	uint64_t volumeProductNonzeroPixels = 0;
	uint64_t volumeRawHash = 0;
	uint64_t volumeRawNonzeroPixels = 0;
	double volumeRawMinimum = 0.0;
	double volumeRawMaximum = 0.0;
	uint8_t volumeProductMinimum = 255;
	uint8_t volumeProductMaximum = 0;
	uint32_t volumeProductMinX = 0;
	uint32_t volumeProductMinY = 0;
	uint32_t volumeProductMaxX = 0;
	uint32_t volumeProductMaxY = 0;
	uint64_t offPreTonemapHash = 0;
	uint64_t silkPreTonemapHash = 0;
	uint64_t offFinalHash = 0;
	uint64_t silkFinalHash = 0;
	uint64_t offShadowHash = 0;
	uint64_t silkShadowHash = 0;
	uint64_t offDepthHash = 0;
	uint64_t silkDepthHash = 0;
	uint64_t offNormalHash = 0;
	uint64_t silkNormalHash = 0;
	uint64_t offShadowAtlasHash = 0;
	uint64_t silkShadowAtlasHash = 0;
	uint64_t offAoHash = 0;
	uint64_t silkAoHash = 0;
	uint64_t offBentNormalHash = 0;
	uint64_t silkBentNormalHash = 0;
};

struct TrinityStandaloneEngineDiagnostics
{
	bool valid = false;
	bool configured = false;
	bool enabled = false;
	bool ready = false;
	uint32_t boosterCount = 0;
	uint32_t renderableCount = 0;
	uint32_t glowCount = 0;
	uint32_t trailCount = 0;
	uint32_t trailPrimitiveCount = 0;
	uint32_t plumeBatchCount = 0;
	uint32_t plumeInstanceCount = 0;
	uint32_t glowSubmissionCount = 0;
	uint32_t trailBatchCount = 0;
	uint32_t trailInstanceCount = 0;
	uint32_t lightSubmissionCount = 0;
	float throttle = 0.0f;
	float nativeIntensity = 0.0f;
	float intensity = 0.0f;
	float trailIntensity = 0.0f;
	float trailLength = 0.0f;
	bool boostersVisible = false;
	bool trailsVisible = false;
	bool highLod = false;
	uint64_t offPreTonemapHash = 0;
	uint64_t authoredPreTonemapHash = 0;
	uint64_t offBloomHash = 0;
	uint64_t authoredBloomHash = 0;
	uint64_t offFinalHash = 0;
	uint64_t authoredFinalHash = 0;
	uint64_t offDepthHash = 0;
	uint64_t authoredDepthHash = 0;
	uint64_t offNormalHash = 0;
	uint64_t authoredNormalHash = 0;
	uint64_t offDistortionHash = 0;
	uint64_t authoredDistortionHash = 0;
	uint64_t offShadowHash = 0;
	uint64_t authoredShadowHash = 0;
	uint64_t offShadowAtlasHash = 0;
	uint64_t authoredShadowAtlasHash = 0;
	uint64_t offAoHash = 0;
	uint64_t authoredAoHash = 0;
	uint64_t offBentNormalHash = 0;
	uint64_t authoredBentNormalHash = 0;
};

extern "C" bool TrinityStandaloneProbeConfigurePostProcess(
	void* opaqueProbe,
	int dynamicExposureMode,
	int bloomMode,
	int filmGrainMode,
	bool diagnosticsEnabled );
extern "C" bool TrinityStandaloneProbeSetExposureCameraPhase(
	void* opaqueProbe,
	bool sequenceActive,
	bool sunFacing );
extern "C" bool TrinityStandaloneProbeGetPostProcessDiagnostics(
	void* opaqueProbe,
	TrinityStandalonePostProcessDiagnostics* diagnostics );
extern "C" bool TrinityStandaloneProbeGetToneValidation(
	void* opaqueProbe,
	TrinityStandaloneToneValidation* validation );
extern "C" bool TrinityStandaloneProbeGetPostFinishValidation(
	void* opaqueProbe,
	TrinityStandalonePostFinishValidation* validation );
extern "C" bool TrinityStandaloneProbeConfigureDistortion( void* opaqueProbe, int distortionMode );
extern "C" bool TrinityStandaloneProbeGetDistortionDiagnostics(
	void* opaqueProbe,
	TrinityStandaloneDistortionDiagnostics* diagnostics );
extern "C" bool TrinityStandaloneProbeValidateDistortion( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeConfigureVolumetrics(
	void* opaqueProbe,
	int mode,
	int quality,
	uint32_t seed );
extern "C" bool TrinityStandaloneProbeSetSilkEnabled( void* opaqueProbe, bool enabled );
extern "C" bool TrinityStandaloneProbeGetVolumetricDiagnostics(
	void* opaqueProbe,
	TrinityStandaloneVolumetricDiagnostics* diagnostics );
extern "C" bool TrinityStandaloneProbeValidateVolumetrics( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeSetEnginesEnabled( void* opaqueProbe, bool enabled );
extern "C" bool TrinityStandaloneProbeGetEngineDiagnostics(
	void* opaqueProbe,
	TrinityStandaloneEngineDiagnostics* diagnostics );
extern "C" bool TrinityStandaloneProbeValidateEngines( void* opaqueProbe );
