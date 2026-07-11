// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GltfToCmfOptions
{
	std::string inputPath;
	std::string sourceName;
	bool verbose = false;
	bool mergeMeshes = false;
};

struct GltfToCmfTexture
{
	std::string name;
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> rgba;
};

struct GltfToCmfResult
{
	std::vector<uint8_t> cmfBytes;
	std::vector<std::string> warnings;
	std::vector<GltfToCmfTexture> baseColorTextures;
	uint32_t meshCount = 0;
	uint32_t skeletonCount = 0;
	std::vector<std::string> animationNames;
};

bool BuildCmfFromGltf( const GltfToCmfOptions& options, GltfToCmfResult& result, std::string& error );
