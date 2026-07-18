// Copyright © 2026 CCP ehf.

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class IRoot;

// A read-only, deterministic view of the Blue-owned portion of a live scene.
// The standalone probe augments this with renderer state which is not owned by
// Blue (variable publications, batch counters, and retained products).
struct TrinityStandaloneSceneGraphSnapshot
{
	struct Root
	{
		std::string role;
		IRoot* object = nullptr;
	};

	std::string json;
	uint64_t normalizedHash = 0;
	uint32_t nodeCount = 0;
	uint32_t edgeCount = 0;
	uint32_t omissionCount = 0;
};

TrinityStandaloneSceneGraphSnapshot TrinityStandaloneCaptureBlueGraph(
	const std::vector<TrinityStandaloneSceneGraphSnapshot::Root>& roots );
