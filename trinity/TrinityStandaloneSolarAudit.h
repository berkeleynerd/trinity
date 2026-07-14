// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>
#include <memory>
#include <string>

class EvePlanet;
class EveSpaceScene;

// Sample-only, asset-free census support for the standalone New Eden probe.
// The implementation retains logical identifiers and authored scalar values;
// it never copies resource payloads into the report.
class TrinityStandaloneSolarAudit
{
public:
	TrinityStandaloneSolarAudit();
	~TrinityStandaloneSolarAudit();

	bool CaptureSourceGraph( EvePlanet& sun, std::string& error );
	void CapturePreparedGraph( EvePlanet& sun );
	bool WriteReport(
		const char* reportPath,
		EvePlanet* preparedSun,
		EveSpaceScene& scene,
		uint64_t renderedFrames,
		std::string& error );

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
