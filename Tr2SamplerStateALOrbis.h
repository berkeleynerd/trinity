#pragma once
#ifndef Tr2SamplerStateALOrbis_H
#define Tr2SamplerStateALOrbis_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include <gnm/sampler.h>

class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();
	ALResult Create(
		Tr2RenderContextAL& renderContext,
		const Tr2SamplerDescription& description );
	bool IsValid() const;

	bool operator==( const Tr2SamplerStateAL& ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	bool m_isValid;
	sce::Gnm::Sampler m_sampler;

	friend class Tr2RenderContextAL;
};

#endif

#endif
