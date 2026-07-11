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
