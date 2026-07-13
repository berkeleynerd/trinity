// Copyright (c) 2026 CCP ehf.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Tr2MetalResourceBindingDiagnostic
{
	std::string name;
	std::string kind;
	uint32_t index = 0;
	uint32_t access = 0;
	uint32_t textureType = 0;
	uint32_t textureUsage = 0;
	uint64_t requiredBytes = 0;
	uint64_t boundBytes = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;
	bool isNil = false;
	bool isDummy = false;
	bool valid = false;
	std::string failure;
};

struct Tr2MetalEncoderDiagnostic
{
	std::string label;
	int32_t errorState = 0;
	std::vector<std::string> debugSignposts;
};

struct Tr2MetalSubmissionDiagnostics
{
	int32_t commandStatus = 0;
	int32_t errorCode = 0;
	std::string errorDomain;
	std::string errorDescription;
	double cpuEncodeSeconds = 0.0;
	double cpuWaitSeconds = 0.0;
	double gpuStartTime = 0.0;
	double gpuEndTime = 0.0;
	uint64_t pipelineUid = 0;
	bool validationEnabled = false;
	bool bindingPreflightPassed = false;
	std::vector<Tr2MetalEncoderDiagnostic> encoders;
	std::vector<Tr2MetalResourceBindingDiagnostic> bindings;
};
