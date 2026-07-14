// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

class IRoot;
class EvePlanet;
class EveSpaceScene;
class Tr2PostProcess2;

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
	void CaptureEnvironmentPostProcess( Tr2PostProcess2& postProcess );
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

// Shared report helpers keep PL-14B/C's sample-only evidence on the same
// reflection path as the accepted PL-14A census.
void TrinityStandaloneWriteAuthoredScalarAttributesJson( std::ostream& output, IRoot* object );
void TrinityStandaloneWriteSolarRootAnimationJson( std::ostream& output, EvePlanet& sun );
