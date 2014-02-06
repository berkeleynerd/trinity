#pragma once
#ifndef Tr2CapsALOrbis_H
#define Tr2CapsALOrbis_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

class Tr2CapsAL
{
public:
	bool SupportsFloat16() const;
	bool SupportsGpuBuffer() const;
	bool SupportsStandaloneSwapChain() const;
	uint32_t GetShaderVersion() const;
	bool SupportsVertexShaderTextures() const;
private:
	Tr2CapsAL();
	Tr2CapsAL( const Tr2CapsAL& );
	Tr2CapsAL& operator=( const Tr2CapsAL& );

	friend class Tr2PrimaryRenderContextAL;
};

#endif

#endif